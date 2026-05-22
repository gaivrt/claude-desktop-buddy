---
title: flash_character.py
type: tool
source: tools/flash_character.py
updated: 2026-05-22
---

# `flash_character.py` — USB-flash a prepped pack

Bypasses BLE folder push by writing the character into `data/characters/<name>/` and running `pio run -t uploadfs`. Faster for iteration when you have USB access.

## Usage

```bash
python3 tools/flash_character.py characters/bufo
```

## Pipeline

1. Verify `manifest.json` exists (run `prep_character.py` first if not)
2. Sum file sizes; reject if > `1_800_000` bytes
3. **Wipe `data/characters/`** entirely — only one character on device
4. `copytree(src, data/characters/<name>)`
5. `pio run -t uploadfs` in project root (raises on failure)
6. Prints: `flashed. on the stick: hold A -> settings -> species -> GIF`

## Why wipe `data/`

`uploadfs` flashes the entire LittleFS image from `data/`. A stale sibling under `data/characters/` would also land on the device, wasting partition space (the firmware only reads one at a time).

## See Also

- [prep_character.py](prep_character.md) — pipeline that creates the input
- [Character pack format](../concepts/character-pack.md) — schema
- [PlatformIO config](../decisions/protocol-vs-implementation.md) — LittleFS / no_ota partitioning
