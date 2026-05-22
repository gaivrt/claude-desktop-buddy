---
title: Buddy Module (ASCII species renderer)
type: module
source: src/buddy.{cpp,h}, src/buddy_common.h
updated: 2026-05-22
---

# `buddy` — ASCII multi-species renderer

## Responsibility

Renders one of 18 ASCII pet species. Each species lives in `src/buddies/<name>.cpp` and exposes 7 `StateFn` callbacks (one per [persona state](../concepts/seven-states.md)). This module owns species registry, tick gating, peek/landscape scaling, and shared text-drawing helpers.

## API

```cpp
void buddyInit();
void buddyTick(uint8_t personaState);             // per-loop, ticks at 5Hz
void buddyRenderTo(TFT_eSPI* tgt, uint8_t state); // one-shot to arbitrary surface
void buddySetSpecies(const char* name);
void buddySetSpeciesIdx(uint8_t idx);
void buddyNextSpecies();                          // cycles + persists
void buddySetPeek(bool peek);                     // toggles 1× vs 2× scale
void buddyInvalidate();                           // force redraw next tick
uint8_t buddySpeciesIdx();
uint8_t buddySpeciesCount();
const char* buddySpeciesName();
```

## Species shape

```cpp
typedef void (*StateFn)(uint32_t t);

struct Species {
  const char* name;
  uint16_t bodyColor;
  StateFn states[7];   // sleep, idle, busy, attention, celebrate, dizzy, heart
};
```

Registered species (`SPECIES_TABLE` in `buddy.cpp`, 18 entries):
`capybara, duck, goose, blob, cat, dragon, octopus, owl, penguin, turtle, snail, ghost, axolotl, cactus, robot, rabbit, mushroom, chonk`.

## Tick rate & gating

- `TICK_MS = 200` (5 fps animation)
- Loop runs ~60fps; redraw is identical between ticks → gate on `(tickCount, state, species)` change
- State changes force redraw mid-tick for instant transitions

## Rendering helpers (buddy_common.h)

```cpp
void buddyPrintLine(const char* line, int yPx, uint16_t color, int xOff = 0);
void buddyPrintSprite(const char* const* lines, uint8_t n, int yOffset, uint16_t color, int xOff = 0);
void buddySetCursor(int x, int y);
void buddySetColor(uint16_t fg);
void buddyPrint(const char* s);
```

Geometry constants (shared by all species):
- `BUDDY_X_CENTER = 67`, `BUDDY_CANVAS_W = 135`
- `BUDDY_Y_BASE = 30`, `BUDDY_Y_OVERLAY = 6`
- `BUDDY_CHAR_W = 6`, `BUDDY_CHAR_H = 8` (size-1 glyph)

Shared colors: `BUDDY_BG`, `BUDDY_HEART`, `BUDDY_DIM`, `BUDDY_YEL`, `BUDDY_WHITE`, `BUDDY_CYAN`, `BUDDY_GREEN`, `BUDDY_PURPLE`, `BUDDY_RED`, `BUDDY_BLUE`.

## Scale modes

| Mode             | Scale | Use case                            |
| ---------------- | ----- | ----------------------------------- |
| Home (normal)    | 2×    | Big buddy on home screen            |
| Peek (PET/INFO)  | 1×    | Small buddy at top of info/pet screens |
| Landscape clock  | 1×    | Direct render to `M5.Lcd`           |

At 2× scale `buddyPrintLine` trims trailing/leading spaces so padding doesn't push ink off-screen.

## Render target

`_tgt` defaults to the global `spr`. `buddyRenderTo()` retargets to any `TFT_eSPI*` (used for landscape clock direct-to-LCD).

## Persistence

`buddyInit()` loads `speciesIdxLoad()` from NVS. `buddyNextSpecies()` cycles + `speciesIdxSave`.

## See Also

- [Buddies index](../buddies/index.md) — the 18 species
- [Seven states](../concepts/seven-states.md)
- [Character module](character.md) — GIF alternative
- [Stats / NVS](stats.md) — `speciesIdxLoad/Save`
- [Main / UI](main.md) — `nextPet()` cycle GIF→species→GIF
