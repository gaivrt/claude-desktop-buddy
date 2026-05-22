---
title: prep_character.py
type: tool
source: tools/prep_character.py
updated: 2026-05-22
---

# `prep_character.py` — normalize a character pack

Standardizes source GIFs to 96px-wide with a **consistent cross-state crop**, so the character is the same scale in every animation.

## Usage

```bash
python3 tools/prep_character.py <character-dir-or-zip>
```

Accepts a directory or a `.zip` (auto-extracted; looks for `manifest.json` inside).

## Pipeline

1. **Load**: parses `manifest.json`, expands single-string / array `states` entries
2. **Normalize**: resizes every frame of every state to `REF_W = 1000` width, RGBA
3. **Cross-state bbox**: unions `getbbox()` of every frame across every state → one crop rectangle
4. **Crop + resize**: each frame cropped to global bbox, resized to `TARGET_W = 96`
5. **Flatten alpha**: composited over `colors.bg` RGB
6. **Palette**: `convert("P", palette=Image.ADAPTIVE, colors=64)`
7. **Save**: writes GIFs back with original durations, `disposal=1`, `loop=0`, `optimize=False`
8. **Manifest**: regenerates with normalized filenames (`<state>.gif` or `<state>_0.gif, _1.gif, ...`)

## Size cap warning

If output total > `1800KB`:
```
warning: over 1800KB — desktop install will reject it
gifsicle not found: brew install gifsicle / apt install gifsicle / winget install LCDF.Gifsicle
shrink: gifsicle --batch --lossy=80 -O3 --colors 64 *.gif
```

## Output location

`characters/<name>/` in repo root. Wipes existing folder first.

## Why cross-state bbox

Different source GIFs have different padding around the character. Per-GIF cropping would produce different on-device sizes per state — the pet would visually grow/shrink as states change. One union bbox ensures consistency.

## Why `disposal=1`

GIF disposal mode 1 = leave canvas as-is between frames. Combined with no optimization, every frame paints the full rectangle → simple transparent-as-bg rendering on device. Matches the assumption in [character.cpp](../firmware/character.md) `gifDrawCb`.

## See Also

- [Character pack format](../concepts/character-pack.md)
- [flash_character.py](flash_character.md) — flash the prepped pack via USB
- [Folder push (protocol)](../protocol/folder-push.md) — wireless install alternative
