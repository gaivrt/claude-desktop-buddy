---
title: Mood, Fed, Energy
type: concept
updated: 2026-05-22
---

# Mood, Fed, Energy

The three derived gauges shown on the PET screen.

## Mood — 0..4 tier

Velocity-driven, deny-ratio penalized.

```cpp
vel = statsMedianVelocity();    // median of velocity[8] ring, 0 if empty

if      (vel == 0)   tier = 2;  // no data: neutral
else if (vel < 15)   tier = 4;
else if (vel < 30)   tier = 3;
else if (vel < 60)   tier = 2;
else if (vel < 120)  tier = 1;
else                 tier = 0;

if (approvals + denials >= 3) {
  if (denials > approvals)       tier -= 2;
  else if (denials*2 > approvals) tier -= 1;   // > 33% deny rate
}
clamp(tier, 0, 4);
```

Approve fast (under 15s median) → tier 4. Lots of denials → tier drops.

Render: 4 hearts; `i < mood` filled. Color: ≥3 = RED, ≥2 = HOT (orange), else dim.

## Fed — 0..9 pips

Token progress within the current level. Pure derivation, no separate state:

```cpp
fed = (tokens % 50000) / 5000;
```

See [levels-xp.md](levels-xp.md).

## Energy — 0..5 tiers

Starts at 3/5 on boot. Refills to full on nap end. Drains 1 tier per **2 hours** since last nap.

```cpp
hoursSince = (millis() - _lastNapEndMs) / 3600000;
e = _energyAtNap - hoursSince / 2;
clamp(e, 0, 5);
```

Render: 5 bars. Color: ≥4 cyan, ≥2 yellow, else HOT.

## See Also

- [Stats module](../firmware/stats.md)
- [Screens](screens.md) — PET screen layout
- [Levels & XP](levels-xp.md)
