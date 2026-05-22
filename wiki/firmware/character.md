---
title: Character Module (GIF + text mode rendering)
type: module
source: src/character.{cpp,h}
updated: 2026-05-22
---

# `character` — GIF/text-mode pet renderer

## Responsibility

Loads a character pack from `/characters/<name>/` on LittleFS, parses its `manifest.json`, and renders the active state into either the global sprite or an arbitrary `TFT_eSPI` surface (for landscape clock).

## API

```cpp
bool  characterInit(const char* name);  // null name → scan /characters/ for first dir
bool  characterLoaded();
void  characterSetState(uint8_t s);     // 0..6, see seven-states
void  characterTick();                  // per-loop frame advance
void  characterInvalidate();            // full clear + reopen
void  characterClose();
void  characterSetPeek(bool peek);      // half-scale, top-pinned
void  characterRenderTo(TFT_eSPI* tgt, int cx, int cy);  // landscape clock
const Palette& characterPalette();
```

## Manifest schema

```json
{
  "name": "bufo",
  "colors": { "body": "#…", "bg": "#…", "text": "#…", "textDim": "#…", "ink": "#…" },
  "mode": "text" | (omitted = gif mode),
  "states": {
    "sleep": "sleep.gif",
    "idle":  ["idle_0.gif", "idle_1.gif", ...],
    ...
  }
}
```

Full schema in [character-pack.md](../concepts/character-pack.md).

## Two modes

### GIF mode (default)

- Up to 32 GIF paths cached per character (`MAX_GIFS`)
- Each state has `stateStart`/`stateCount`/`stateRot` — rotates through array variants every `VARIANT_DWELL_MS` (5s)
- Single-GIF states freeze on last frame (avoid LittleFS-reopen storm starving BT controller)
- Multi-GIF states loop current GIF until dwell elapses, then advance with `ANIM_PAUSE_MS` (800ms) pause
- Decode: `AnimatedGIF` lib (`bitbank2/AnimatedGIF ^2.1.1`); LittleFS-backed file callbacks
- Transparent pixels painted with `pal.bg` — assumes GIFs are unoptimized full-frame (gifsicle `--unoptimize`)

### Text mode (`"mode":"text"`)

- States contain `{frames:[...], delay:N}` — short ASCII strings rendered at text size 2, centered
- Frame rotation per state at `delayMs` interval
- Clears only a band around the text — leaves HUD/overlays alone
- Use case: lightweight characters without GIF assets

## Geometry

| Mode                    | Layout                                                              |
| ----------------------- | ------------------------------------------------------------------- |
| Home (normal)           | Centered in upper 140px of 135×240; full scale                      |
| Peek (PET/INFO screens) | Half scale (2:1 NN downscale), bottom-pinned to `PEEK_TOP=70`       |
| Landscape clock         | `characterRenderTo(&M5.Lcd, cx, cy)` — half-scale to arbitrary point |

## Color palette

`parseHexColor` converts `#RRGGBB` → RGB565 (16-bit). Defaults: body=`0xC2A6`, bg=`0x0000`, text=`0xFFFF`, textDim=`0x8410`, ink=`0x0000`.

## Boot scan

`characterInit(nullptr)` scans `/characters/` and loads the first subdirectory found. Whatever you last installed is what boots.

## See Also

- [Character pack format](../concepts/character-pack.md)
- [Seven states](../concepts/seven-states.md)
- [Main / UI](main.md) — owner of the loop calling `characterTick`
- [Xfer module](xfer.md) — installer that writes `/characters/<name>/`
- [Buddy module](buddy.md) — alternative ASCII renderer for the same 7 states
