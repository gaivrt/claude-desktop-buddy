---
title: ASCII Buddies (18 species)
type: buddy
source: src/buddies/*.cpp
updated: 2026-05-22
---

# ASCII Buddies — 18 Species

Each species lives in `src/buddies/<name>.cpp` (~11-12 KB each) and exposes a `Species` struct registered in `src/buddy.cpp`. The renderer is shared — see [buddy module](../firmware/buddy.md).

## Species table

`SPECIES_TABLE` in `src/buddy.cpp`, index order matters (persisted in NVS `species` key):

| Idx | Name      | File                          |
| --- | --------- | ----------------------------- |
| 0   | capybara  | `src/buddies/capybara.cpp`    |
| 1   | duck      | `src/buddies/duck.cpp`        |
| 2   | goose     | `src/buddies/goose.cpp`       |
| 3   | blob      | `src/buddies/blob.cpp`        |
| 4   | cat       | `src/buddies/cat.cpp`         |
| 5   | dragon    | `src/buddies/dragon.cpp`      |
| 6   | octopus   | `src/buddies/octopus.cpp`     |
| 7   | owl       | `src/buddies/owl.cpp`         |
| 8   | penguin   | `src/buddies/penguin.cpp`     |
| 9   | turtle    | `src/buddies/turtle.cpp`      |
| 10  | snail     | `src/buddies/snail.cpp`       |
| 11  | ghost     | `src/buddies/ghost.cpp`       |
| 12  | axolotl   | `src/buddies/axolotl.cpp`     |
| 13  | cactus    | `src/buddies/cactus.cpp`      |
| 14  | robot     | `src/buddies/robot.cpp`       |
| 15  | rabbit    | `src/buddies/rabbit.cpp`      |
| 16  | mushroom  | `src/buddies/mushroom.cpp`    |
| 17  | chonk     | `src/buddies/chonk.cpp`       |

Plus sentinel `0xFF` → GIF mode (when a GIF character is installed).

## Per-species shape

```cpp
const Species CAPYBARA_SPECIES = {
  .name = "capybara",
  .bodyColor = 0x...,         // RGB565
  .states = {
    sleep_fn, idle_fn, busy_fn, attention_fn,
    celebrate_fn, dizzy_fn, heart_fn,
  },
};
```

Each `StateFn(uint32_t t)` receives the global tickCount (5fps) and renders into the shared sprite (or retargeted surface). Uses helpers from `buddy_common.h`:

- `buddyPrintSprite(lines, n, yOffset, color, xOff)` — multi-line text block
- `buddyPrintLine(line, yPx, color, xOff)`
- `buddySetCursor / buddySetColor / buddyPrint`

All species share `BUDDY_X_CENTER=67` horizontally. Scale switches between 1× (peek/landscape) and 2× (home) via `_scale` static.

## Build inclusion

`platformio.ini` build src filter:

```ini
build_src_filter = +<*> +<buddies/>
```

So all `buddies/*.cpp` compile in.

## Cycling

Settings → `ascii pet` cycles GIF (if available) → 0 → 1 → ... → 17 → GIF. `nextPet()` in [main.cpp](../firmware/main.md).

## Example GIF pack

`characters/bufo/` is the reference GIF character pack — 15 GIFs (9 idle variants + 6 single-state) mapped to the same 7 states. See [character-pack.md](../concepts/character-pack.md) for the manifest format.

> bufo's GIFs come from the community **bufo** emoji set at [bufo.zone](https://bufo.zone) and are **not** covered by this repo's MIT license — see top-level `LICENSE`.

## See Also

- [Buddy module](../firmware/buddy.md) — renderer details
- [Seven states](../concepts/seven-states.md) — what each state means
- [Character pack format](../concepts/character-pack.md) — GIF alternative
