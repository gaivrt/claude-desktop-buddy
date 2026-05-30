#!/usr/bin/env python3
"""Buddy Voice companion — the Buddy's mic becomes Chinese dictation.

Reads framed audio from the Buddy over USB serial, transcribes each utterance
with FunASR, and types the Chinese into the focused window (Claude desktop).
Long-lived: FunASR models load once at startup, then every utterance is fast.

    uv run --with funasr --with torch --with torchaudio --with pyserial \
        python -u companion/main.py --port COM5

Frame: 0xAA 0x55 | type(1) | len(2 LE) | payload(len) | checksum(1)
  0x01 REC_START   0x02 AUDIO(pcm)   0x03 REC_END   0x04 SUBMIT
The sync word + checksum let us skip the buddy's interleaved debug/JSON text and
the 115200 boot banner we can't read at 921600.
"""
import argparse, os, sys, tempfile, time, wave
import serial
from serial.tools import list_ports

import inject

sys.stdout.reconfigure(encoding="utf-8", errors="replace")

BAUD = 115200
SR = 16000
SYNC = bytes((0xAA, 0x55))
F_START, F_AUDIO, F_END, F_SUBMIT = 0x01, 0x02, 0x03, 0x04
MIN_UTTER_BYTES = SR * 2 * 3 // 10   # ignore <0.3s blips

# IMA ADPCM — must match src/voice_capture.h exactly.
IMA_STEP = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767]
IMA_IDX = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]


def decode_adpcm(block):
    """Decode one self-contained IMA-ADPCM block → 16-bit PCM bytes."""
    if len(block) < 3:
        return b""
    pred = int.from_bytes(block[0:2], "little", signed=True)
    idx = block[2]
    out = bytearray()
    for byte in block[3:]:
        for nib in (byte & 0x0F, byte >> 4):
            step = IMA_STEP[idx]
            diffq = step >> 3
            if nib & 4:
                diffq += step
            if nib & 2:
                diffq += step >> 1
            if nib & 1:
                diffq += step >> 2
            pred = pred - diffq if (nib & 8) else pred + diffq
            pred = -32768 if pred < -32768 else (32767 if pred > 32767 else pred)
            idx += IMA_IDX[nib]
            idx = 0 if idx < 0 else (88 if idx > 88 else idx)
            out += int(pred).to_bytes(2, "little", signed=True)
    return bytes(out)


def say(*a):
    print(*a, flush=True)


def find_port():
    for p in list_ports.comports():
        if p.vid == 0x0403 and p.pid == 0x6001:   # FTDI FT232R = the Buddy
            return p.device
    return None


def load_asr():
    say("[asr] loading FunASR (one-time model load ~50s) ...")
    t0 = time.time()
    from funasr import AutoModel
    model = AutoModel(model="paraformer-zh", vad_model="fsmn-vad",
                      punc_model="ct-punc", disable_update=True)
    say(f"[asr] ready in {time.time() - t0:.0f}s")
    return model


def transcribe(model, pcm):
    # FunASR wants 16k/16-bit mono; a temp WAV is the proven path (Phase 0b).
    fd, path = tempfile.mkstemp(suffix=".wav")
    os.close(fd)
    try:
        with wave.open(path, "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(SR)
            w.writeframes(bytes(pcm))
        res = model.generate(input=path)
        return res[0].get("text", "") if res else ""
    finally:
        os.remove(path)


class FrameParser:
    """Streaming sync-word parser; skips interleaved debug text / boot garbage."""

    def __init__(self):
        self.buf = bytearray()

    def feed(self, data):
        self.buf += data
        out = []
        while True:
            i = self.buf.find(SYNC)
            if i < 0:
                # nothing yet; keep a trailing byte (a lone 0xAA may begin a sync)
                if len(self.buf) > 1:
                    del self.buf[:-1]
                return out
            if i > 0:
                del self.buf[:i]                 # drop garbage before the sync
            if len(self.buf) < 5:
                return out                       # need type + len
            typ = self.buf[2]
            ln = self.buf[3] | (self.buf[4] << 8)
            # Reject implausible frames (a stray 0xAA55 inside debug/boot garbage
            # or raw PCM) before waiting on a bogus length: type must be known,
            # control frames carry no payload, AUDIO is at most one chunk.
            ok = typ in (F_START, F_AUDIO, F_END, F_SUBMIT) and \
                (ln <= 512 if typ == F_AUDIO else ln == 0)
            if not ok:
                del self.buf[:2]                 # resync past this false sync
                continue
            need = 5 + ln + 1
            if len(self.buf) < need:
                return out                       # wait for the full frame
            payload = bytes(self.buf[5:5 + ln])
            calc = (typ + self.buf[3] + self.buf[4] + sum(payload)) & 0xFF
            if calc == self.buf[5 + ln]:
                out.append((typ, payload))
                del self.buf[:need]
            else:
                del self.buf[:2]                 # bad frame: resync past the sync


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default=None)
    args = ap.parse_args()
    model = load_asr()

    def open_serial():
        # Wait for the Buddy and open it (retry forever). Survives the device
        # not being plugged in yet — e.g. when this is auto-started at login.
        warned = False
        while True:
            p = args.port or find_port()
            if p:
                try:
                    sp = serial.Serial()
                    sp.port = p
                    sp.baudrate = BAUD
                    sp.timeout = 0.05
                    sp.dtr = False
                    sp.rts = False
                    sp.open()
                    return sp
                except Exception:
                    pass
            if not warned:
                say("[serial] waiting for the Buddy on USB ...")
                warned = True
            time.sleep(2.0)

    s = open_serial()
    say(f"[serial] listening @ {BAUD}")
    say("ready — focus Claude desktop's message box, hold B on the Buddy and talk.\n")

    parser = FrameParser()
    pcm = bytearray()
    recording = False

    while True:
        try:
            data = s.read(4096)
        except (serial.SerialException, OSError) as e:
            # USB/FTDI glitches drop the handle; recover instead of dying.
            say(f"[serial] lost ({e}); reconnecting ...")
            try:
                s.close()
            except Exception:
                pass
            parser = FrameParser()
            pcm = bytearray()
            recording = False
            s = open_serial()       # blocks until the Buddy is back
            say("[serial] reconnected")
            continue
        if not data:
            continue
        for typ, payload in parser.feed(data):
            if typ == F_START:
                pcm = bytearray()
                recording = True
                say("● REC")
            elif typ == F_AUDIO:
                if recording:
                    pcm += decode_adpcm(payload)
            elif typ == F_END:
                recording = False
                dur = len(pcm) / (SR * 2)
                if len(pcm) >= MIN_UTTER_BYTES:
                    say(f"■ stop ({dur:.1f}s) — transcribing ...")
                    t0 = time.time()
                    text = transcribe(model, pcm)
                    say(f'  → "{text}"  ({time.time() - t0:.1f}s)')
                    if text:
                        inject.type_unicode(text)
                        time.sleep(0.05)      # let the text settle in the box
                        inject.press_enter()  # auto-send the message
                else:
                    say(f"■ stop ({dur:.1f}s) — too short, ignored")
                pcm = bytearray()
            elif typ == F_SUBMIT:
                recording = False
                pcm = bytearray()
                say("⏎ submit")
                inject.press_enter()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        say("\nstopped.")
