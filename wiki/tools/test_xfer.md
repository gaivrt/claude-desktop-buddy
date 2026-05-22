---
title: test_xfer.py
type: tool
source: tools/test_xfer.py
updated: 2026-05-22
---

# `test_xfer.py` — drive folder push from USB

Validates the `xfer.h` receiver by streaming a character pack over serial. Watches every ack, verifies the device reloads.

## Usage

```bash
python3 tools/test_xfer.py                              # uses ../characters/bufo, name "test"
python3 tools/test_xfer.py <src-dir>                    # custom source
python3 tools/test_xfer.py <src-dir> <name>             # custom name
```

## Protocol sequence (matches [folder-push](../protocol/folder-push.md))

1. `char_begin` with `name` (retries up to 8× at 1s intervals until acked)
2. For each file in source:
   - `file` with `path` + `size`
   - Loop: `chunk` with base64-encoded 256-byte slice, wait `chunk` ack
   - `file_end`, validate `n == size`
3. `char_end`

## Chunk size

`CHUNK = 256` bytes (pre-base64). Matches the device's UART RX buffer comfortably; each chunk is acked before the next is sent (matches xfer.h's per-chunk ack discipline).

## Ack parser

`wait_ack(what, timeout)` reads lines, skips non-JSON, returns the first `{"ack": what, ...}` it finds. Non-ack noise is logged with `(skip: ...)`.

## Bootstrap quirk

```python
s.dtr = True; s.rts = False
time.sleep(2)   # let any DTR-triggered reset finish booting
```

USB DTR-toggle resets some ESP32 boards. The 2s sleep + input-buffer reset before sending lets the boot greeting flush out.

## Throughput report

Prints `<total> bytes in <dt>s = <KB/s>` on success. Typical: ~3-8 KB/s over serial (LittleFS write speed bounded).

## See Also

- [Folder Push (protocol)](../protocol/folder-push.md)
- [Xfer module](../firmware/xfer.md) — receiver
- [Character pack format](../concepts/character-pack.md)
