---
title: Protocol vs Implementation Split
type: decision
updated: 2026-05-22
---

# Protocol vs Implementation Split

## Decision

The repo has two distinct layers with different stability guarantees:

| Layer        | Artifact                  | Stability                  | Audience           |
| ------------ | ------------------------- | -------------------------- | ------------------ |
| Protocol     | `REFERENCE.md`            | Stable, versioned by Claude | Any device builder |
| Reference    | `src/`, `tools/`, `characters/` | Just one example           | M5StickC users     |

## Why

Anyone with a BLE-capable board (ESP32, nRF52, RPi+BLE dongle) can implement the protocol with **zero code from this repo**. The repo exists to:

1. Document the protocol with a working impl that proves it
2. Make it cheap to get *something* running for makers who want a working device today

Conflating the two would constrain the protocol to whatever the M5StickC firmware happens to do.

## Concrete consequences

- **`build_src_filter = +<*> +<buddies/>`** in `platformio.ini` — explicitly opt-in to species; a fork can drop them
- **`board_build.partitions = no_ota.csv`** — sacrifice OTA to maximize user partition (1.8MB cap on character packs). A fork with different priorities can change this
- **`board_build.filesystem = littlefs`** — chosen for the character pack folder push; not a protocol requirement
- **Library deps** (`M5StickCPlus`, `AnimatedGIF`, `ArduinoJson`) — implementation details. The protocol only requires "parse JSON + speak BLE NUS"
- **Reference impl adds non-spec commands** (`species`, `char_begin/file/chunk/file_end/char_end`) — fork-specific extensions. See [commands-acks](../protocol/commands-acks.md#reference-impl-extras-non-spec)

## Non-spec extensions

A fork can layer additional services on the same BLE peripheral without touching the protocol contract. This repo's Windows-target fork adds:

- **BLE HID keyboard** (`src/ble_hid.{cpp,h}`) — **removed.** Once drove a Win+H dictation trigger, but advertising as a keyboard made Windows grab the device and starve Claude desktop's NUS link, so `hidInit()` is no longer called. Files kept but unwired. See [BLE HID module](../firmware/ble-hid.md).
- **Voice ASR via Buddy mic** (`src/voice_capture.h` + a PC companion) — record on the Buddy's own SPM1423 mic, IMA-ADPCM over USB serial, transcribe with FunASR, inject Chinese via SendInput into the focused window. The active fork feature. See [Voice ASR](voice-asr.md) and [Voice Capture](../firmware/voice-capture.md).

These are **not** required for protocol compliance — a device that only advertises NUS and parses heartbeat JSON still works as a Hardware Buddy.

## Wiki structure encodes the split

- `wiki/protocol/` is authoritative
- `wiki/firmware/` is annotated example
- Conflicts: protocol wins; firmware page gets `⚠ Conflict with protocol` annotation

## See Also

- [Governance](governance.md)
- [Overview](../overview.md)
- [REFERENCE.md](../../REFERENCE.md)
