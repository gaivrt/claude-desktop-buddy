---
title: Levels & XP (token-driven)
type: concept
updated: 2026-05-22
---

# Levels & XP

Tokens are the XP currency. Level increments every **50,000 cumulative output tokens**.

```cpp
static const uint32_t TOKENS_PER_LEVEL = 50000;
```

## Mechanics

- Bridge sends `tokens` in every heartbeat — cumulative since the desktop app started
- Device tracks **deltas** to handle bridge restart (drop → resync) and device reboot (latch on first sight)
- Tokens accumulate in RAM; NVS write only when crossing a level boundary

```cpp
lvlBefore = tokens / TOKENS_PER_LEVEL;
tokens   += delta;
lvlAfter  = tokens / TOKENS_PER_LEVEL;

if (lvlAfter > lvlBefore) {
  level = lvlAfter;
  _levelUpPending = true;     // one-shot edge
  _dirty = true; statsSave();
}
```

## Level-up celebration

`main.cpp` polls `statsPollLevelUp()` each loop; on `true` → `triggerOneShot(P_CELEBRATE, 3000)`.

## Fed bar

The pet's "fed" pip bar (10 pips) on the PET screen tracks progress *within* the current level:

```cpp
statsFedProgress() = (tokens % TOKENS_PER_LEVEL) / (TOKENS_PER_LEVEL / 10);
                   = (tokens % 50000) / 5000;     // 0..9
```

Each pip = 5,000 tokens.

## Loss on hard power-off

Worst case 50,000 tokens lost (one full level minus 1). Acceptable tradeoff vs writing NVS on every heartbeat (would burn NVS sectors in days).

## Backfill on boot

If NVS has `level > 0` but `tokens == 0` (corruption or migration), `statsLoad` synthesizes `tokens = level * TOKENS_PER_LEVEL` so the derivation holds.

## See Also

- [Stats module](../firmware/stats.md)
- [Heartbeat (protocol)](../protocol/heartbeat.md) — `tokens` field
- [Seven states](seven-states.md) — `celebrate` is also triggered by `recentlyCompleted` flag
- [NVS Layout](nvs-layout.md)
