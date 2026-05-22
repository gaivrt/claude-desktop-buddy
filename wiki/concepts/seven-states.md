---
title: Seven Persona States
type: concept
updated: 2026-05-22
---

# Seven Persona States

The whole UI vocabulary. Both the GIF renderer ([character](../firmware/character.md)) and the ASCII renderer ([buddy](../firmware/buddy.md)) implement exactly these seven, indexed 0..6.

| Idx | State        | Trigger                                                   | Feel                        |
| --- | ------------ | --------------------------------------------------------- | --------------------------- |
| 0   | `sleep`      | bridge not connected (`!tama.connected`)                  | eyes closed, slow breathing |
| 1   | `idle`       | connected, nothing urgent                                 | blinking, looking around    |
| 2   | `busy`       | `sessionsRunning >= 3`                                    | sweating, working           |
| 3   | `attention`  | `sessionsWaiting > 0` (approval pending)                  | alert, **LED blinks**       |
| 4   | `celebrate`  | level up (every 50K tokens) or `recentlyCompleted` flag   | confetti, bouncing          |
| 5   | `dizzy`      | shake detected (Δ|accel| > 0.8g)                          | spiral eyes, wobbling       |
| 6   | `heart`      | approved within 5s                                        | floating hearts             |

## Base vs active

`baseState` is derived from `TamaState` each loop. `activeState` is what renders — a `triggerOneShot(state, durMs)` overrides base for N ms.

```cpp
PersonaState derive(const TamaState& s) {
  if (!s.connected)            return P_IDLE;   // see note
  if (s.sessionsWaiting > 0)   return P_ATTENTION;
  if (s.recentlyCompleted)     return P_CELEBRATE;
  if (s.sessionsRunning >= 3)  return P_BUSY;
  return P_IDLE;
}
```

> **Quirk**: when bridge is disconnected, `derive()` returns `P_IDLE` but `data.h` clears `msg` to `"No Claude connected"` and reports `connected=false`. The sleep state shows up via the wake-transition hold (`baseState = P_SLEEP` for 12s after wake from `screenOff`).

## One-shots

| One-shot     | Duration | Triggered by                                             |
| ------------ | -------- | -------------------------------------------------------- |
| `P_HEART`    | 2s       | approval responded under 5s                              |
| `P_DIZZY`    | 2s       | shake check (`checkShake()` magnitude > 0.8)             |
| `P_CELEBRATE`| 3s       | `statsPollLevelUp()` edge                                |

## Clock-face overrides

When the [clock face](clock-face.md) is active, `activeState` is set directly based on time-of-day/day-of-week (e.g., 1-7am → sleep; weekend → mostly sleep with occasional hearts; Friday after 3pm → celebrate). Bypasses `derive()`.

## See Also

- [Character module](../firmware/character.md) — `STATE_NAMES[]` array + `characterSetState`
- [Buddy module](../firmware/buddy.md) — `Species.states[7]`
- [Main / UI](../firmware/main.md) — `derive()`, `triggerOneShot()`
- [Clock Face](clock-face.md) — overrides
