---
title: Xfer Module (folder push + side commands)
type: module
source: src/xfer.h
updated: 2026-05-22
---

# `xfer.h` — folder push receiver + non-heartbeat commands

## Responsibility

Handles all `cmd`-bearing JSON messages: the folder push state machine plus orthogonal commands (`name`, `species`, `unpair`, `owner`, `status`). Header-only with file-static state.

## Entry point

```cpp
bool xferCommand(JsonDocument& doc);   // true = handled, caller skips heartbeat parse
bool xferActive();                     // mid-folder-push
uint32_t xferProgress();               // bytes written so far
uint32_t xferTotal();                  // expected total from char_begin
```

`data.h::_applyJson` calls `xferCommand` first; if it returns true, no further parsing.

## Per-command behavior

| `cmd`         | Action                                                                                                  |
| ------------- | ------------------------------------------------------------------------------------------------------- |
| `name`        | `petNameSet(name)` → NVS                                                                                |
| `species`     | `speciesIdxSave(idx)`; toggles `buddyMode` based on whether GIF available & `idx == 0xFF`               |
| `unpair`      | `bleClearBonds()`                                                                                       |
| `owner`       | `ownerSet(name)` → NVS                                                                                  |
| `status`      | Builds JSON ack with battery, system, stats, fs (see [commands-acks](../protocol/commands-acks.md))      |
| `char_begin`  | Fit check → wipe `/characters/` → mkdir → arm transfer                                                  |
| `file`        | Open `/characters/<charname>/<path>` for write                                                          |
| `chunk`       | Base64-decode (max 300B), write, ack with bytes-so-far                                                  |
| `file_end`    | Close, validate `_xWritten == _xExpected || _xExpected == 0`                                            |
| `char_end`    | `characterInit(name)`; on success: `buddyMode = false; gifAvailable = true; speciesIdxSave(0xFF)`        |

If a non-transfer command arrives mid-transfer it's still handled (state isolated by `_xActive` flag).

## Fit check (`char_begin`)

```
free        = LittleFS total - used
reclaimable = sum of all sizes under /characters/
available   = free + reclaimable
need        = total + 4096   // metadata headroom
```

If `need > available`: ack `ok:false`, embed `"error":"need NK, have NK"`, **don't touch the filesystem** — preserves current character.

## Ack mechanism

All acks go to **both** Serial and BLE via `_xAck`. Writes to clientless SerialBT drop silently — the active transport gets the response.

## Acks every chunk — why

LittleFS writes can block during flash erase, and the UART RX buffer is ~256 bytes. Without per-chunk acks the sender overruns it. Acks include `_xWritten` so the desktop can show progress.

## Status response data

| Field           | Source                                          |
| --------------- | ----------------------------------------------- |
| `name`          | `petName()`                                     |
| `owner`         | `ownerName()`                                   |
| `sec`           | `bleSecure()`                                   |
| `bat.pct`       | `(vBat - 3200) / 10` clamped 0..100             |
| `bat.mV/mA`     | `M5.Axp.GetBatVoltage/Current`                  |
| `bat.usb`       | `vBus > 4000`                                   |
| `sys.up`        | `millis() / 1000`                               |
| `sys.heap`      | `ESP.getFreeHeap()`                             |
| `sys.fsFree`    | `LittleFS.totalBytes - usedBytes`               |
| `sys.fsTotal`   | `LittleFS.totalBytes`                           |
| `stats.appr`    | `stats().approvals`                             |
| `stats.deny`    | `stats().denials`                               |
| `stats.vel`     | `statsMedianVelocity()`                         |
| `stats.nap`     | `stats().napSeconds`                            |
| `stats.lvl`     | `stats().level`                                 |

## See Also

- [Folder Push (protocol)](../protocol/folder-push.md) — wire format
- [Commands & Acks (protocol)](../protocol/commands-acks.md) — `name/owner/status/unpair`
- [Data module](data.md) — dispatcher
- [Character module](character.md) — `characterInit/Close` consumers
- [Stats / NVS](stats.md) — `petNameSet/ownerSet/speciesIdxSave`
