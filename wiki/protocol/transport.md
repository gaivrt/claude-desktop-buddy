---
title: BLE Transport (Nordic UART Service)
type: protocol-msg
source: REFERENCE.md
updated: 2026-05-22
---

# Transport — Nordic UART Service

Wire transport is the **BLE Nordic UART Service** (NUS), the de-facto serial-over-BLE standard. Any tool that speaks NUS (nRF Connect, bluefy, Web Bluetooth examples) can talk to the device with no custom configuration.

## UUIDs

|                               | UUID                                   |
| ----------------------------- | -------------------------------------- |
| Service                       | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` |
| RX (desktop → device, write)  | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` |
| TX (device → desktop, notify) | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` |

## Advertising

- Advertise a name starting with `Claude` so the desktop picker can filter
- Appending a few bytes of BT MAC (e.g. `Claude-AB12`) disambiguates multiple sticks
- Advertise the NUS service UUID

## Frame format

- UTF-8 JSON, **one object per line, terminated with `\n`**
- Multi-packet lines may fragment at the MTU boundary; just send bytes — the desktop reassembles
- Device must do the same: accumulate bytes until `\n`, then parse

## MTU

- Default 23 bytes; macOS typically negotiates to ~185 bytes
- ATT notify payload = MTU − 3
- Reference impl caps chunks at 180 bytes regardless of MTU (see `firmware/ble-bridge.md`)

## See Also

- `REFERENCE.md` — original prose
- [BLE Bridge module](../firmware/ble-bridge.md)
- [Security & Pairing](security.md)
