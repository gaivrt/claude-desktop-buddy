---
title: Turn Events
type: protocol-msg
source: REFERENCE.md
updated: 2026-05-22
---

# Turn Events (desktop → device)

Each completed turn fires a one-shot event containing the raw SDK content array — text blocks, tool calls, and any other content from the message.

```json
{
  "evt": "turn",
  "role": "assistant",
  "content": [{ "type": "text", "text": "..." }]
}
```

## Size limit

Events that serialize **larger than 4KB** are dropped (measured in UTF-8 bytes, not character count).

> **Magic number**: 4KB cap — central source of truth for this constant lives here.

## Notes

The reference firmware does not consume `evt:"turn"` events in `data.h` — the `entries` field of the heartbeat snapshot is what populates the on-screen transcript. Turn events are a richer companion stream for devices that want structured content.

## See Also

- [Heartbeat Snapshot](heartbeat.md)
