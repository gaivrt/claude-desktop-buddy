---
title: Character Pack Format
type: concept
updated: 2026-05-22
---

# Character Pack Format

A folder + `manifest.json` + a set of 96px-wide GIFs (or strings for text mode), installed under `/characters/<name>/` on LittleFS.

## Folder structure

```
my-character/
├── manifest.json
├── sleep.gif
├── idle.gif        (or idle_0.gif, idle_1.gif, ... — array form)
├── busy.gif
├── attention.gif
├── celebrate.gif
├── dizzy.gif
└── heart.gif
```

## Manifest

```json
{
  "name": "bufo",
  "colors": {
    "body": "#6B8E23",
    "bg": "#000000",
    "text": "#FFFFFF",
    "textDim": "#808080",
    "ink": "#000000"
  },
  "states": {
    "sleep": "sleep.gif",
    "idle": ["idle_0.gif", "idle_1.gif", "idle_2.gif"],
    "busy": "busy.gif",
    "attention": "attention.gif",
    "celebrate": "celebrate.gif",
    "dizzy": "dizzy.gif",
    "heart": "heart.gif"
  }
}
```

- `colors`: hex strings, converted to RGB565 on device. See `parseHexColor` in [character.cpp](../firmware/character.md).
- `states`: keys must be from `[sleep, idle, busy, attention, celebrate, dizzy, heart]`. Value: filename or array.
- **Arrays rotate** on loop-end (every 5s `VARIANT_DWELL_MS`), then advance with 800ms pause. Useful for idle carousel.

## Text mode (lightweight alternative)

```json
{
  "name": "minimal",
  "colors": { ... },
  "mode": "text",
  "states": {
    "idle": { "frames": ["( . _ . )", "( - _ - )"], "delay": 400 },
    ...
  }
}
```

- Renders short strings at text size 2, centered, no GIF pipeline
- `delay` in ms (default 200)
- Up to 8 frames per state, 19 chars per frame

## Size constraints

| Constraint              | Value          | Source                          |
| ----------------------- | -------------- | ------------------------------- |
| Total folder size       | **≤ 1.8 MB**   | LittleFS partition cap          |
| GIF width               | 96 px          | `TARGET_W` in prep_character.py |
| GIF height              | ≤ ~140 px      | Display constraint              |
| Max GIFs per pack       | 32             | `MAX_GIFS` in character.cpp     |
| `_xCharName` length     | 23 chars       | xfer.h buffer                   |
| Per-chunk decode buffer | 300 bytes      | xfer.h base64 decode            |

`gifsicle --lossy=80 -O3 --colors 64` typically cuts 40-60%.

## Geometry & transparency

- Crop tight to the character — transparent margins waste screen + shrink the rendered sprite
- GIFs must be **unoptimized full-frame** (gifsicle `--unoptimize --lossy`) — the character pipeline treats transparent = bg color; disposal semantics are encoder-dependent
- `tools/prep_character.py` handles normalize → cross-state bbox → consistent crop → 96px resize → 64-color palette

## Distribution paths

1. **Wireless** — drag folder onto Hardware Buddy window → BLE folder push
2. **USB** — `python3 tools/flash_character.py characters/<name>` → `pio run -t uploadfs`
3. **Repo** — drop in `characters/<name>/` and prep via tools

## Installation behavior

On `char_begin`:
1. Pre-flight space check (current chars are reclaimable)
2. If fits: `characterClose()` → wipe `/characters/` → mkdir new
3. Files streamed via [folder push](../protocol/folder-push.md)
4. On `char_end`: `characterInit(name)`, flip to GIF mode (`speciesIdxSave(0xFF)`)

Only one character lives on device at a time.

## Boot scan

`characterInit(nullptr)` scans `/characters/` and loads the first subdirectory. Whatever you last installed boots.

## See Also

- [Folder Push (protocol)](../protocol/folder-push.md)
- [Character module](../firmware/character.md)
- [prep_character tool](../tools/prep_character.md)
- [flash_character tool](../tools/flash_character.md)
- [bufo example](../buddies/index.md#example-gif-pack) — reference pack
