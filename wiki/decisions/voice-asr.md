---
title: Voice ASR via Buddy Mic (Chinese dictation into Claude desktop)
type: decision
source: src/voice_capture.h, companion/, docs/chinese-voice-input.html
updated: 2026-05-30
---

# Voice ASR via Buddy Mic — 方案 B (shipped)

A **fork-specific feature**, integrated into the main firmware: hold the Buddy's
**B** button on the Voice screen, speak into the Buddy's own microphone, release,
and the Chinese transcription is typed **and auto-sent** into the focused window
— in practice the **Claude desktop** message box. Not part of the published BLE
protocol; see [protocol-vs-implementation](protocol-vs-implementation.md#non-spec-extensions).

End-user setup guide: `docs/chinese-voice-input.html`.

## Context

User wanted 中文语音输入. Two readings:

- **方案 A** — Buddy as a remote trigger: B sends `Win+H` over a BLE HID keyboard,
  the **PC's** mic + Windows dictation do the work. Rejected — user wants to talk
  to the Buddy itself. This path (and its HID keyboard) has since been **removed**;
  see [ASR Integration](../concepts/asr-integration.md).
- **方案 B (shipped)** — Buddy's **own** mic records, transcription runs off-device,
  Chinese is injected into Claude desktop.

## Three hard facts that force the architecture

1. **ESP32 can't run ASR locally** (ESP32-PICO-D4, 520KB SRAM, no PSRAM) → audio
   **must leave the device** for transcription.
2. **No WiFi** (user constraint). Audio leaves over **USB serial**.
3. **CJK can't be typed by a BLE HID keyboard** — a boot keyboard sends US keycodes,
   not 「你好」. Arbitrary Unicode into a focused Windows app needs `SendInput` +
   `KEYEVENTF_UNICODE`, which **only a PC-side program can do**.

→ Facts 1+3 force a **PC companion** (Python). HID is not just unnecessary — it was
removed (see below).

## Architecture (as shipped)

```
┌──────── Buddy (M5StickC Plus) ────────┐        ┌──── PC Companion (Python/uv) ────┐
│ hold B → I2S reads SPM1423 PDM mic     │        │  read framed serial              │
│   16kHz/16-bit → IMA-ADPCM 4:1         ├─USB───▶│  → IMA-ADPCM decode → 16k PCM    │
│   → framed over USB serial @ 115200    │ 115200 │  → FunASR (paraformer-zh+vad+punc)│
│ release B → REC_END                    │        │  → SendInput KEYEVENTF_UNICODE   │
│ tap B (<300ms) → SUBMIT                │        │     into focused window          │
└────────────────────────────────────────┘        │  → press Enter (auto-send)       │
   firmware module: src/voice_capture.h            └──────────────────────────────────┘
```

Button (Voice screen only): **hold B = record**; **release = transcribe + type +
auto-send**; **quick tap (<300ms) = Enter**. Other screens keep B's normal behavior.

## Two findings that shaped the final design

### ① 115200 + IMA-ADPCM, *not* a high baud

Real-time 16kHz/16-bit is 256 kbps, so the plan was `921600`. But **every high baud
garbled on this board's FTDI** (921600, 1 Mbaud, at 160 and 240 MHz CPU) — only
`115200` is reliable. So audio is **IMA-ADPCM compressed 4:1** (256 → ~64 kbps) to
fit 115200's ~92 kbps usable. ADPCM loss is negligible for FunASR. ⚠ This is a
**hardware quirk of this board** — a board whose high baud works could skip ADPCM
and stream raw PCM.

### ② BLE HID keyboard removed

The earlier fork advertised a BLE HID keyboard (for 方案 A's Win+H). On Windows that
made the OS **grab the device as a keyboard, starving Claude desktop's NUS/GATT
connection** — "pairs then immediately disconnects". 方案 B doesn't need HID, so
`hidInit()`/`hidTick()` were removed from `startBt`/`loop`; the device is now pure
NUS and Claude desktop connects cleanly. `src/ble_hid.{cpp,h}` remain but are unwired.

## Component facts

- **Mic** — SPM1423 PDM. I2S: `WS/CLK = GPIO0`, `DATA = GPIO34`, 16kHz mono, 16-bit.
- **Transport** — `0xAA 0x55 | type(1) | len(2 LE) | payload | checksum(1)`; types
  `REC_START / AUDIO / REC_END / SUBMIT`. AUDIO payload = `[predictor:int16][index:u8]`
  + ADPCM nibbles (each frame self-syncs → a dropped frame can't cascade). Sync word
  + checksum let the companion skip the device's interleaved debug text.
- **ASR** — **FunASR** (`paraformer-zh` + `fsmn-vad` + `ct-punc`), local/offline.
  Models ~1GB (one-time from ModelScope), load ~50s **per process start** → the
  companion is **long-lived**. Transcribe ~0.6–0.8s per utterance, with punctuation.
- **Injection** — `ctypes` `SendInput` + `KEYEVENTF_UNICODE`. `WinError 87` gotchas:
  the `INPUT` struct must be the full union (`sizeof == 40` on x64) and
  `SendInput.argtypes` must be set. Auto-send presses Enter after typing.
- **Resilience** — companion waits for the Buddy and auto-reconnects on USB glitches.

## Status

- **Phase 0 — de-risk** ✅ (mic / FunASR / injection each validated independently)
- **Phase 1 — shipped** ✅ integrated into the buddy firmware: new Voice screen
  (breathing mic, design-system styled), I2S+ADPCM capture, companion with FunASR +
  inject + auto-send + reconnect, launcher (`start-voice.bat` / `.vbs`), setup guide.
- **Phase 2 — BLE wireless** — **declined.** A BLE audio path would contend with
  Claude desktop for the Buddy's single BLE connection (and ESP32 BLE bandwidth is
  tight for the ~64 kbps audio, likely forcing 8kHz). USB-only is reliable and was
  judged good enough; wireless was not pursued.
- **Phase 3 — polish ideas** — streaming partial results; on-device "transcribing"
  feedback; a macOS companion; re-flashing the GIF character pack (LittleFS).

## Magic numbers

| Value | Meaning |
| ----- | ------- |
| 16000 | sample rate (Hz), FunASR-native |
| GPIO0 / GPIO34 | PDM mic CLK / DATA |
| 115200 | USB serial baud (high bauds unreliable on this FTDI) |
| 4:1 | IMA-ADPCM compression ratio (256 → ~64 kbps) |
| 300 ms | B-hold threshold: tap = Enter, hold = dictate |
| 40 | `sizeof(INPUT)` on x64 — `SendInput` `cbSize` must equal this |
| ~50s | FunASR model load per process start (→ long-lived companion) |

## See Also

- [Voice Capture module](../firmware/voice-capture.md) — the firmware side
- [ASR Integration](../concepts/asr-integration.md) — 方案 A (Win+H/HID), now removed
- [Protocol vs Implementation](protocol-vs-implementation.md#non-spec-extensions)
- [Voice Screen](../concepts/screens.md#voice)
- [Overview](../overview.md)
