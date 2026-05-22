---
title: test_serial.py
type: tool
source: tools/test_serial.py
updated: 2026-05-22
---

# `test_serial.py` — drive states from USB serial

Quick smoke test. Writes 4 heartbeat-ish JSON objects in a cycle, 3s each, 20 iterations. Watch the device react.

## Usage

```bash
python3 tools/test_serial.py
```

No args. Auto-discovers `/dev/cu.usbserial-*` (macOS pattern; **Linux/Windows users would need to edit the glob**).

## Scenarios cycled

```python
{"total": 0, "running": 0, "waiting": 0}   # → sleep
{"total": 2, "running": 1, "waiting": 0}   # → idle
{"total": 4, "running": 3, "waiting": 0}   # → busy
{"total": 2, "running": 1, "waiting": 1}   # → attention, LED blinks
```

These exercise the [persona derivation](../concepts/seven-states.md) decision tree.

## Why USB and not BLE

Reference impl's `data.h` ingests both `Serial` (USB) and `bleAvailable/bleRead` (BLE) through the same `_applyJson` path. Sending over USB is enough to validate the parse + state machine without dealing with BLE pairing.

## See Also

- [Data module](../firmware/data.md) — `_applyJson` consumer
- [Heartbeat (protocol)](../protocol/heartbeat.md) — full field set
- [test_xfer.py](test_xfer.md) — companion that exercises folder push
