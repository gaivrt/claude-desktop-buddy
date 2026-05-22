---
title: One-Shot on Connect
type: protocol-msg
source: REFERENCE.md
updated: 2026-05-22
---

# One-Shot on Connect (desktop → device)

After the link comes up, the desktop sends a few one-shot messages.

## Time sync

```json
{ "time": [1775731234, -25200] }
```

- `[0]` epoch seconds
- `[1]` timezone offset in **seconds** east of UTC

Reference impl applies via `data.h`:
- Compute `local = epoch + tz_offset`, then `gmtime_r(&local, &lt)`
- Write to `M5.Rtc` (Hours/Mins/Secs + WeekDay/Month/Date/Year)
- Set `_rtcValid = true` (gates the [clock face](../concepts/clock-face.md))
- Zero `_clkLastRead` so the next render picks up the new time immediately

## Owner

```json
{ "cmd": "owner", "name": "Felix" }
```

Sets the owner name (user's first name from their account). Reference impl persists to NVS via `ownerSet()` and renders it on the boot greeting + Pet screen header (`Felix's Buddy`).

## See Also

- [Commands & Acks](commands-acks.md) — `owner` is also a regular command
- [Data module](../firmware/data.md) — `_applyJson` dispatch
- [Stats / NVS](../firmware/stats.md) — `ownerSet` / `ownerName`
