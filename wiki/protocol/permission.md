---
title: Permission Decisions
type: protocol-msg
source: REFERENCE.md
updated: 2026-05-22
---

# Permission Decisions (device → desktop)

When a heartbeat snapshot includes a `prompt` object, the device can return a response.

## Wire format

```json
{"cmd":"permission","id":"req_abc123","decision":"once"}
{"cmd":"permission","id":"req_abc123","decision":"deny"}
```

- `id` **must match `prompt.id` exactly**
- `decision` is `"once"` (approve) or `"deny"` (reject)
- Desktop forwards this to the session manager

## Reference implementation

Triggered from `main.cpp` button handlers:
- **A** (front) when `inPrompt` → `decision:"once"` + `statsOnApproval(seconds)`; emits `P_HEART` one-shot if responded within 5s
- **B** (right) when `inPrompt` → `decision:"deny"` + `statsOnDenial()`
- `responseSent` flag flips so the panel shows "sent..." until the desktop drops `prompt`
- Prompt arrival raises `promptArrivedMs` for the response-time stat; alert chirp at 1200Hz; LED pulses while `activeState == P_ATTENTION`

## See Also

- [Heartbeat Snapshot](heartbeat.md) — where `prompt` arrives
- [Main / UI](../firmware/main.md) — approval screen rendering and button handlers
- [Stats / NVS](../firmware/stats.md) — `statsOnApproval`, `statsOnDenial`
