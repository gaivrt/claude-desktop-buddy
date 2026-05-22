---
title: Data Module (TamaState + JSON dispatch)
type: module
source: src/data.h
updated: 2026-05-22
---

# `data.h` — line buffer, JSON dispatch, TamaState

## Responsibility

Pulls newline-delimited JSON off USB Serial and BLE, parses it, and folds it into a single in-memory state struct that the UI reads. Implements **three modes** with strict priority:

1. **demo** — auto-cycle fake scenarios every 8s, ignore live data
2. **live** — JSON arrived in the last 10s over USB or BLE
3. **asleep** — no data, all zeros, `"No Claude connected"`

Header-only with file-static state — include from one TU only.

## TamaState

```cpp
struct TamaState {
  uint8_t  sessionsTotal, sessionsRunning, sessionsWaiting;
  bool     recentlyCompleted;
  uint32_t tokensToday, lastUpdated;
  char     msg[24];
  bool     connected;
  char     lines[8][92];        // transcript, newest at end
  uint8_t  nLines;
  uint16_t lineGen;             // bumps when lines change → resets scroll
  char     promptId[40];
  char     promptTool[20];
  char     promptHint[44];
};
```

## API

```cpp
void dataPoll(TamaState* out);     // call every loop iter
bool dataConnected();              // last live within 30s
bool dataBtActive();               // BLE byte seen within 15s
const char* dataScenarioName();    // "demo"|"bt"|"usb"|"none"
bool dataRtcValid();               // RTC has been time-synced
void dataSetDemo(bool on);
bool dataDemo();
```

## JSON dispatch (`_applyJson`)

Decision order matters:

1. Try `xferCommand(doc)` first (folder push + side commands handled in `xfer.h`)
2. Try `time` array → time sync → set RTC + `_rtcValid`
3. Otherwise: merge heartbeat fields (`total/running/waiting/completed/tokens/tokens_today/msg/entries/prompt`) into `out`

Field mechanics:
- `tokens` is cumulative-since-bridge-start; forwarded to `statsOnBridgeTokens()` (level tracking)
- `entries` array: copies up to 8 lines × 91 chars; bumps `lineGen` when content changes
- `prompt` object: copies `id/tool/hint`; if absent, clears all three (closes approval screen)

## Line buffer

Generic `_LineBuf<1024>` template. USB uses `Stream::available/read`. BLE uses `bleAvailable/bleRead` (not a Stream). Newline or carriage return terminates a line; only lines starting with `{` are passed to `_applyJson`.

## See Also

- [Heartbeat (protocol)](../protocol/heartbeat.md) — field semantics
- [On-Connect (protocol)](../protocol/on-connect.md) — time sync semantics
- [Xfer module](xfer.md) — sibling command dispatch
- [Stats / NVS](stats.md) — `statsOnBridgeTokens` token tracking
- [Main / UI](main.md) — consumer of TamaState
