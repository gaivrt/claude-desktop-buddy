---
title: Clock Face
type: concept
updated: 2026-05-22
---

# Clock Face

Takes over the home screen when the device is **charging on USB** with nothing else going on.

## Entry conditions (all must hold)

- `displayMode == DISP_NORMAL`
- No overlay (menu/settings/reset closed)
- No prompt
- `tama.sessionsRunning == 0 && tama.sessionsWaiting == 0`
- `dataRtcValid()` — RTC has been synced via [on-connect time](../protocol/on-connect.md)
- `_onUsb` — `M5.Axp.GetVBusVoltage() > 4.0V`

Bridge heartbeat alone is **not** activity (it's how we got the RTC synced in the first place).

## Two orientations

| `clockOrient` | Description                                      |
| ------------- | ------------------------------------------------ |
| 0             | Portrait — sprite path, pet peeks at top         |
| 1             | Landscape, BtnA-side down — `M5.Lcd.setRotation(1)` |
| 3             | Landscape, USB-side down — `M5.Lcd.setRotation(3)` |

Gravity-detected via IMU `ax` axis. Hysteresis (dual threshold): strict to enter (≥15 frames at |ax|>0.7), loose to stay (-8 frames at |ax|<0.4). Direct 1↔3 swap path for fast flips that never cross zero.

User can lock via `settings.clockRot`: 0=auto, 1=portrait, 2=landscape (1↔3 still flips from gravity).

## Portrait layout

Sprite path. Buddy peeks at top (half-scale) above `y=90`. Clock below:
- `HH:MM` size 4 at y=140
- `:SS` size 2 at y=175
- `Mon DD` size 1 at y=200

## Landscape layout

Direct-to-LCD (sprite untouched). 240×135 rotated.
- Pet on left at 5fps, redraws with `fillRect(0,0,115,90)` clear
- Right: `HH:MM` size 3 + `SS` + day-of-week / month / date stack
- Seconds re-render gated on `_clkTm.Seconds != lastSec` (avoids 180 SPI ops/sec for nothing)

## Pet state override

When clocking, `activeState` is set by time-of-day, bypassing `derive()`:

| Time            | Behavior                                                          |
| --------------- | ----------------------------------------------------------------- |
| 1-7am           | `P_SLEEP`                                                         |
| Weekend         | Mostly `P_SLEEP`, occasional `P_HEART` (every ~48s)               |
| Before 9am      | Mostly `P_SLEEP`, occasional `P_IDLE`                             |
| 12:00           | Mostly `P_IDLE`, occasional `P_HEART` (lunchtime)                 |
| Friday ≥3pm     | Mostly `P_IDLE`, occasional `P_CELEBRATE`                         |
| ≥22:00 or 0:00  | Mostly `P_SLEEP`, occasional `P_DIZZY`                            |
| otherwise       | Mostly `P_IDLE`, occasional `P_SLEEP`                             |

## RTC caching

RTC and IMU share the I2C bus. Reading RTC at 60fps starves IMU reads → noisy orientation detection. `clockRefreshRtc()` caches the RTC at 1Hz; mood logic and `drawClock` read from the cached `_clkTm`/`_clkDt`.

## Screen-off override

Auto-off (30s idle) is **disabled on USB power** so the clock stays visible while charging.

## See Also

- [On-Connect (protocol)](../protocol/on-connect.md) — time sync that gates `dataRtcValid`
- [Main / UI](../firmware/main.md) — `drawClock`, `clockUpdateOrient`, `clockRefreshRtc`
- [Seven states](seven-states.md)
- [Screens](screens.md)
