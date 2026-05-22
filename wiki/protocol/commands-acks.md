---
title: Commands & Acks
type: protocol-msg
source: REFERENCE.md
updated: 2026-05-22
---

# Commands & Acks

Any command the desktop sends with a `cmd` field expects a matching ack:

```json
{ "ack": "<same as cmd>", "ok": true, "n": 0 }
```

Set `ok:false` and optionally `error:"..."` if you couldn't do it. `n` is a generic counter (e.g. bytes written for chunk acks, otherwise 0).

## Spec-defined commands

| Command                          | Payload                  | Ack you send back            |
| -------------------------------- | ------------------------ | ---------------------------- |
| `{"cmd":"status"}`               | —                        | see Status response below    |
| `{"cmd":"name","name":"Clawd"}`  | sets device display name | `{"ack":"name","ok":true}`   |
| `{"cmd":"owner","name":"Felix"}` | sets owner name          | `{"ack":"owner","ok":true}`  |
| `{"cmd":"unpair"}`               | erase stored BLE bonds   | `{"ack":"unpair","ok":true}` |

## Status response

```json
{
  "ack": "status",
  "ok": true,
  "data": {
    "name": "Clawd",
    "sec": true,
    "bat": { "pct": 87, "mV": 4012, "mA": -120, "usb": true },
    "sys": { "up": 8412, "heap": 84200 },
    "stats": { "appr": 42, "deny": 3, "vel": 8, "nap": 12, "lvl": 5 }
  }
}
```

- Desktop polls every couple of seconds to populate the Hardware Buddy window stats panel
- Omit fields you don't have
- `bat.mA` negative means charging

## Reference impl extras (non-spec)

`xfer.h` handles two additional commands the reference firmware accepts. These are **not part of the public protocol** — fork-specific:

| Command                              | Effect                                                                |
| ------------------------------------ | --------------------------------------------------------------------- |
| `{"cmd":"species","idx":N}`          | Switch ASCII species by index; `0xFF` = use installed GIF             |
| `{"cmd":"permission","id":...,...}`  | **device → desktop** direction; xfer.h passes this through unchanged  |

Reference status ack also includes additional `data` fields:
- `owner` (string)
- `bat.usb` (bool: VBUS > 4V)
- `sys.fsFree`, `sys.fsTotal` (LittleFS bytes)

## See Also

- [Xfer module](../firmware/xfer.md) — command dispatch
- [Stats / NVS](../firmware/stats.md) — fields backing `appr/deny/vel/nap/lvl`
- [Security & Pairing](security.md) — `sec:true` semantics + `unpair`
