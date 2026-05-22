---
title: Heartbeat Snapshot
type: protocol-msg
source: REFERENCE.md
updated: 2026-05-22
---

# Heartbeat Snapshot (desktop → device)

Sent whenever desktop state changes, plus a **keepalive every 10 seconds**.

```json
{
  "total": 3,
  "running": 1,
  "waiting": 1,
  "msg": "approve: Bash",
  "entries": ["10:42 git push", "10:41 yarn test", "10:39 reading file..."],
  "tokens": 184502,
  "tokens_today": 31200,
  "prompt": {
    "id": "req_abc123",
    "tool": "Bash",
    "hint": "rm -rf /tmp/foo"
  }
}
```

## Fields

| Field          | Meaning                                                                           |
| -------------- | --------------------------------------------------------------------------------- |
| `total`        | Count of all sessions                                                             |
| `running`      | Sessions actively generating                                                      |
| `waiting`      | Sessions blocked on a permission prompt                                           |
| `msg`          | One-line summary suitable for a small display                                     |
| `entries`      | Recent transcript lines, newest first (capped to a few)                           |
| `tokens`       | Cumulative output tokens since the desktop app started                            |
| `tokens_today` | Output tokens since local midnight (persisted, survives restart)                  |
| `prompt`       | Only present when a permission decision is needed. The `id` is what you echo back |

## Derived signals

- `running > 0` → at least one session is generating
- `waiting > 0` → a permission prompt is blocking
- `total == 0` → nothing is open
- `tokens_today` resets at local midnight (daily counter convenience)

## Timeout

If no snapshot for **~30 seconds**, treat the connection as dead.

## Implementation notes

The reference impl tracks live state in `TamaState` (see `firmware/data.md`):
- Receipt timestamp drives `dataConnected()` (≤30s window)
- `lineGen` bumps when `entries` changes — lets UI reset transcript scroll
- `prompt.id` change triggers approval screen + alert beep
- `tokens` is cumulative-since-bridge-start; reference impl tracks deltas and increments NVS only on level boundaries (see [Levels & XP](../concepts/levels-xp.md))

## See Also

- [Permission Decisions](permission.md) — what to do when `prompt` is present
- [Turn Events](turn-event.md) — companion event for completed turns
- [Data module](../firmware/data.md) — line buffer + JSON dispatch
