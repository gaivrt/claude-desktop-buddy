---
title: Folder Push
type: protocol-msg
source: REFERENCE.md
updated: 2026-05-22
---

# Folder Push (desktop → device)

The Hardware Buddy window has a drop target. Dropping a folder streams its flat contents to the device. The transport is content-agnostic — the reference impl uses it for character packs, but you can ship GIFs, config blobs, firmware images, anything under **1.8MB**.

## Sequence

```
desktop:  {"cmd":"char_begin","name":"bufo","total":184320}
device:   {"ack":"char_begin","ok":true}

desktop:  {"cmd":"file","path":"manifest.json","size":412}
device:   {"ack":"file","ok":true}
desktop:  {"cmd":"chunk","d":"<base64>"}
device:   {"ack":"chunk","ok":true,"n":<bytes_written_so_far>}
          ...repeat chunk until file is done...
desktop:  {"cmd":"file_end"}
device:   {"ack":"file_end","ok":true,"n":<final_size>}

          ...repeat file/chunk/file_end for each file...

desktop:  {"cmd":"char_end"}
device:   {"ack":"char_end","ok":true}
```

## Properties

- **Flat**: no recursion; dotfiles skipped
- **Sequential**: every chunk is acked before the next is sent — no buffering whole files
- **Base64-encoded chunks**: in the `d` field
- **Naming**: `char_begin.name` is the folder name *unless* the folder contains `manifest.json` with a `"name"` field, which wins
- **Reject mechanism**: don't ack `char_begin` if you don't want pushed files; desktop times out after a few seconds

## Magic numbers

- **1.8MB** total folder size cap
- Reference impl decode buffer: 300 bytes per chunk
- Reference impl acks **every** chunk because LittleFS flash erase can block and the UART RX buffer is only ~256 bytes

## Reference impl behavior (xfer.h)

- On `char_begin`: pre-flight free-space check (including bytes reclaimable from existing `/characters/*`); if `total + 4096 > available`, return `ok:false` with `error:"need NK, have NK"` and don't touch the filesystem
- After fit check passes: `characterClose()` → wipe all `/characters/` → `mkdir /characters/<name>`
- Per chunk: base64-decode (max 300 bytes), append to current `_xFile`, ack with running `_xWritten`
- On `file_end`: validate `_xWritten == _xExpected || _xExpected == 0`
- On `char_end`: `characterInit(name)`; if successful, set `gifAvailable = true` and `speciesIdxSave(0xFF)` (GIF mode)

## Security caveat

Validate `file.path` before writing — desktop sends whatever filenames are in the dropped folder. **Reject `..` and absolute paths** unless your filesystem can tolerate overwrites. The reference impl prepends `/characters/<name>/` but doesn't validate `path` further.

## See Also

- [Xfer module](../firmware/xfer.md) — full receiver implementation
- [Character pack format](../concepts/character-pack.md)
- [Test Xfer tool](../tools/test_xfer.md) — Python script that drives this protocol over serial
