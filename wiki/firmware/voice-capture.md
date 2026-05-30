---
title: Voice Capture Module
type: module
source: src/voice_capture.h
updated: 2026-05-30
---

# `voice_capture.h` — SPM1423 mic → IMA-ADPCM → USB serial

## Responsibility

Header-only module (like `data.h`/`xfer.h`) that drives the **Voice screen**'s
Chinese dictation: reads the SPM1423 PDM mic over I2S, compresses to IMA-ADPCM,
and streams framed packets to the PC companion over USB serial. The companion
transcribes (FunASR) and types the result into the focused window. See the
decision page [Voice ASR](../decisions/voice-asr.md) for the full pipeline.

This is a **fork-specific extension**, USB-only — BLE is never touched, so the
Claude-desktop NUS link is unaffected.

## API

```cpp
void     micInit();          // I2S0 PDM @ 16kHz; sets micReady()
bool     micReady();         // I2S init succeeded
void     micStartRec();      // begin: flush DMA, reset ADPCM, send REC_START
void     micStopRec();       // real hold → send REC_END (companion transcribes)
void     micTapSubmit();     // quick tap → send SUBMIT (companion presses Enter)
void     micTick();          // call every loop(): drain DMA, ADPCM, stream
bool     micRecording();
float    micLevel();         // 0..1 smoothed input level (for UI)
uint32_t micElapsedMs();     // recording duration (tap-vs-hold + timer)
```

## Wiring into `main.cpp`

- `setup()` → `micInit()` (after `M5.Beep.begin()`). Serial stays at M5's 115200.
- `loop()` → `micTick()` (next to `clockRefreshRtc()`; no-op unless recording).
- Voice-screen B button: **press** → `micStartRec()`; **release** → if
  `micElapsedMs() < 300` then `micTapSubmit()` else `micStopRec()`. Guarded on
  `micReady() && !xferActive()` and the `swallowBtnB` wake latch.

## Why 115200 + ADPCM

This board's FTDI **garbles every high baud** (921600, 1 Mbaud). Only 115200 is
reliable, but real-time 16kHz/16-bit (256 kbps) doesn't fit it — so audio is
**IMA-ADPCM 4:1** (~64 kbps), which fits 115200's ~92 kbps usable. See the
[decision page](../decisions/voice-asr.md#two-findings-that-shaped-the-final-design).

## Frame protocol

```
0xAA 0x55 | type(1) | len(2 LE) | payload(len) | checksum(1)
  type: 0x01 REC_START  0x02 AUDIO  0x03 REC_END  0x04 SUBMIT
  checksum = (type + len_lo + len_hi + Σpayload) & 0xFF
  AUDIO payload = [predictor:int16 LE][index:u8] + packed ADPCM nibbles
```

Each AUDIO frame is a self-contained IMA-ADPCM block (header carries the
predictor + step index), so a dropped frame can't desync the decoder. The sync
word + checksum let the companion skip the device's interleaved debug prints and
the 115200 boot banner.

## Pins / magic numbers

| Value | Meaning |
| ----- | ------- |
| GPIO0 | PDM mic clock (`ws_io_num`) — also a boot-strap pin, repurposed after boot |
| GPIO34 | PDM mic data (`data_in_num`) |
| 16000 | sample rate, 16-bit mono |
| 256-sample DMA × 4 | I2S DMA buffers; `micTick` drains ≤4 chunks/tick |

## See Also

- [Voice ASR (decision)](../decisions/voice-asr.md) — full pipeline + companion
- [Main / UI](main.md) — `setup`/`loop`/button wiring + the Voice screen
- [BLE HID](ble-hid.md) — the now-removed keyboard 方案 B replaced
- [Voice Screen](../concepts/screens.md#voice)
