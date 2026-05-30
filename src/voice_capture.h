#pragma once
#include <Arduino.h>
#include <driver/i2s.h>

// ---------------------------------------------------------------------------
// Voice capture — SPM1423 PDM mic → 16kHz/16-bit mono PCM, streamed to the PC
// companion over USB serial (UART0) as framed packets:
//
//   0xAA 0x55 | type(1) | len(2 LE) | payload(len) | checksum(1)
//   type: 0x01 REC_START  0x02 AUDIO  0x03 REC_END  0x04 SUBMIT
//   checksum = (type + len_lo + len_hi + sum(payload)) & 0xFF
//
// UART0 is shared with the buddy's debug/JSON traffic, so the sync word + len +
// checksum let the companion skip interleaved text and resync on garbage (incl.
// the 115200 boot banner it can't read at 921600). BLE is never touched — this
// is a USB-only side channel, so the Claude-desktop BLE link is unaffected.
//
// Streaming is drained from the I2S DMA a chunk at a time inside the main loop
// (micTick). AUDIO frames are emitted only when the UART TX buffer has room —
// if it's backed up we DROP the chunk rather than block, so the main loop (and
// thus BLE servicing + UI) is never stalled waiting on the wire. The companion
// tolerates dropped chunks via sync-word resync.
// ---------------------------------------------------------------------------

static const int        MIC_PIN_CLK  = 0;    // SPM1423 PDM clock
static const int        MIC_PIN_DATA = 34;   // SPM1423 PDM data
static const uint32_t   MIC_SR       = 16000;
static const i2s_port_t MIC_PORT     = I2S_NUM_0;
static const size_t     MIC_CHUNK    = 512;  // bytes per AUDIO frame (256 int16)

enum MicFrame : uint8_t { MIC_F_START = 0x01, MIC_F_AUDIO = 0x02,
                          MIC_F_END   = 0x03, MIC_F_SUBMIT = 0x04 };

static bool     _micOk      = false;  // I2S init succeeded
static bool     _micRec     = false;
static uint32_t _micStartMs = 0;
static float    _micLevel   = 0.0f;   // 0..1 smoothed input level for the UI
static int16_t  _micBuf[256];
static uint8_t  _adpcmBuf[140];   // IMA-ADPCM frame: 3-byte header + packed nibbles
static int      _adpcmPred = 0;   // ADPCM predictor, carried across frames
static int      _adpcmIdx  = 0;   // ADPCM step index, carried across frames

static const int16_t _IMA_STEP[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,
    88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,
    544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,
    2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
    10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767 };
static const int8_t _IMA_IDX[16] = { -1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8 };

// IMA ADPCM: 16-bit sample → 4-bit code, updating predictor/index in place.
static uint8_t _imaEncode(int16_t sample) {
  int step = _IMA_STEP[_adpcmIdx];
  int diff = sample - _adpcmPred;
  uint8_t code = 0;
  if (diff < 0) { code = 8; diff = -diff; }
  int t = step;
  if (diff >= t) { code |= 4; diff -= t; }
  t >>= 1;
  if (diff >= t) { code |= 2; diff -= t; }
  t >>= 1;
  if (diff >= t) { code |= 1; }
  int diffq = step >> 3;
  if (code & 4) diffq += step;
  if (code & 2) diffq += step >> 1;
  if (code & 1) diffq += step >> 2;
  _adpcmPred += (code & 8) ? -diffq : diffq;
  if (_adpcmPred > 32767) _adpcmPred = 32767;
  else if (_adpcmPred < -32768) _adpcmPred = -32768;
  _adpcmIdx += _IMA_IDX[code];
  if (_adpcmIdx < 0) _adpcmIdx = 0;
  else if (_adpcmIdx > 88) _adpcmIdx = 88;
  return code;
}

static void _micSend(uint8_t type, const uint8_t* payload, uint16_t len) {
  uint8_t hdr[5] = { 0xAA, 0x55, type, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8) };
  Serial.write(hdr, 5);
  uint8_t sum = type + hdr[3] + hdr[4];
  if (payload && len) {
    Serial.write(payload, len);
    for (uint16_t i = 0; i < len; i++) sum += payload[i];
  }
  Serial.write(sum);
}

inline void micInit() {
  i2s_config_t cfg = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
      .sample_rate          = MIC_SR,
      .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count        = 4,
      .dma_buf_len          = 256,
      .use_apll             = false,
      .tx_desc_auto_clear   = false,
      .fixed_mclk           = 0,
  };
  i2s_pin_config_t pins = {
      .bck_io_num   = I2S_PIN_NO_CHANGE,
      .ws_io_num    = MIC_PIN_CLK,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num  = MIC_PIN_DATA,
  };
  esp_err_t e = i2s_driver_install(MIC_PORT, &cfg, 0, NULL);
  if (e == ESP_OK) e = i2s_set_pin(MIC_PORT, &pins);
  if (e == ESP_OK) e = i2s_set_clk(MIC_PORT, MIC_SR, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  _micOk = (e == ESP_OK);
}

inline bool     micReady()     { return _micOk; }
inline bool     micRecording() { return _micRec; }
inline float    micLevel()     { return _micLevel; }
inline uint32_t micElapsedMs() { return _micRec ? millis() - _micStartMs : 0; }

inline void micStartRec() {
  if (_micRec || !_micOk) return;
  size_t got;   // flush stale DMA so we don't ship the pre-press transient
  for (int i = 0; i < 4; i++) i2s_read(MIC_PORT, _micBuf, sizeof(_micBuf), &got, 0);
  _micRec = true;
  _micStartMs = millis();
  _micLevel = 0.0f;
  _adpcmPred = 0;        // fresh ADPCM state per utterance
  _adpcmIdx = 0;
  _micSend(MIC_F_START, nullptr, 0);
}

inline void micStopRec() {       // a real hold → transcribe on the PC
  if (!_micRec) return;
  _micRec = false;
  _micSend(MIC_F_END, nullptr, 0);
}

inline void micTapSubmit() {     // a quick tap → discard audio, press Enter
  if (!_micRec) return;
  _micRec = false;
  _micSend(MIC_F_SUBMIT, nullptr, 0);
}

// Drain the I2S DMA, IMA-ADPCM compress (4:1), and stream it. Called every
// main-loop tick; bounded read count keeps a burst from stalling the loop.
// 16kHz/16-bit (256kbps) → ADPCM (~64kbps) fits the reliable 115200 link.
inline void micTick() {
  if (!_micRec) return;
  for (int k = 0; k < 4; k++) {
    size_t got = 0;
    if (i2s_read(MIC_PORT, _micBuf, sizeof(_micBuf), &got, 0) != ESP_OK || got == 0) break;
    uint16_t n = got / 2;
    int32_t peak = 0;
    for (uint16_t i = 0; i < n; i++) { int32_t v = _micBuf[i]; if (v < 0) v = -v; if (v > peak) peak = v; }
    _micLevel += (peak / 32768.0f - _micLevel) * 0.3f;
    // Block header = predictor+index BEFORE this block (so the decoder can
    // self-sync each frame, immune to a dropped frame); then packed nibbles.
    _adpcmBuf[0] = (uint8_t)(_adpcmPred & 0xFF);
    _adpcmBuf[1] = (uint8_t)((_adpcmPred >> 8) & 0xFF);
    _adpcmBuf[2] = (uint8_t)_adpcmIdx;
    uint16_t outLen = 3;
    for (uint16_t i = 0; i < n; i += 2) {
      uint8_t lo = _imaEncode(_micBuf[i]);
      uint8_t hi = (i + 1 < n) ? _imaEncode(_micBuf[i + 1]) : 0;
      _adpcmBuf[outLen++] = (uint8_t)((hi << 4) | lo);
    }
    _micSend(MIC_F_AUDIO, _adpcmBuf, outLen);
    if (got < sizeof(_micBuf)) break;   // DMA drained
  }
}
