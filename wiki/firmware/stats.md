---
title: Stats Module (NVS persistence)
type: module
source: src/stats.h
updated: 2026-05-22
---

# `stats.h` — NVS-backed stats, settings, identity

## Responsibility

Owns everything persistent: token-driven levels, approval/denial counts, response velocity, nap time, user settings (sound/BT/wifi/led/HUD/clock rotation), pet name, owner name, species index. Header-only, file-static state — **include from one TU only** (main.cpp).

Save sparingly: NVS sectors have ~100K write cycles. Save on significant events (approval, denial, nap end, level-up); **never on timer**.

## Structs

```cpp
struct Stats {
  uint32_t napSeconds;
  uint16_t approvals, denials;
  uint16_t velocity[8];   // ring of seconds-to-respond
  uint8_t  velIdx, velCount;
  uint8_t  level;
  uint32_t tokens;
};

struct Settings {
  bool sound, bt, wifi, led, hud;
  uint8_t clockRot;       // 0=auto 1=portrait 2=landscape
  uint8_t hostOs;         // 0=mac (Right Cmd), 1=win (Win+H). Default win.
};
```

## NVS keys (namespace `"buddy"`)

| Key       | Type    | Field                          |
| --------- | ------- | ------------------------------ |
| `nap`     | UInt    | `napSeconds`                   |
| `appr`    | UShort  | `approvals`                    |
| `deny`    | UShort  | `denials`                      |
| `vel`     | Bytes   | `velocity[8]` ring             |
| `vidx`    | UChar   | `velIdx`                       |
| `vcnt`    | UChar   | `velCount`                     |
| `lvl`     | UChar   | `level`                        |
| `tok`     | UInt    | `tokens`                       |
| `s_snd`   | Bool    | sound                          |
| `s_bt`    | Bool    | BT preference (BLE always on)  |
| `s_wifi`  | Bool    | wifi (placeholder, no stack)   |
| `s_led`   | Bool    | LED                            |
| `s_hud`   | Bool    | transcript HUD                 |
| `s_crot`  | UChar   | clock rotation                 |
| `s_host`  | UChar   | host OS (0=mac, 1=win)         |
| `petname` | String  | display name                   |
| `owner`   | String  | owner first name               |
| `species` | UChar   | 0..N-1 or 0xFF for GIF mode    |

Centralized in [nvs-layout.md](../concepts/nvs-layout.md).

## API

```cpp
void statsLoad();                              // call once at boot
void statsSave();                              // flush if dirty
void statsOnApproval(uint32_t secondsToRespond);
void statsOnDenial();
void statsOnNapEnd(uint32_t seconds);
void statsOnBridgeTokens(uint32_t bridgeTotal);  // see token-delta tracking
bool statsPollLevelUp();                       // one-shot edge

uint16_t statsMedianVelocity();
uint8_t  statsMoodTier();    // 0..4
uint8_t  statsEnergyTier();  // 0..5
uint8_t  statsFedProgress(); // 0..9, pips on level bar
void     statsOnWake();      // resets energy to full

const Stats& stats();
Settings&    settings();
void settingsLoad/Save();

void petNameLoad/Set();  const char* petName();
void ownerSet();         const char* ownerName();
uint8_t speciesIdxLoad/Save();
```

## Token tracking — bridge restart vs device reboot

Bridge sends cumulative-since-IT-started. Local tracking handles two reset shapes:

```cpp
if (!_tokensSynced) {           // first sight after device boot
  _lastBridgeTokens = bridgeTotal;
  _tokensSynced = true;
  return;                       // don't re-credit on reboot
}
if (bridgeTotal < _lastBridgeTokens) {
  _lastBridgeTokens = bridgeTotal;   // bridge restarted; resync
  return;
}
delta = bridgeTotal - _lastBridgeTokens;
_stats.tokens += delta;
// persist only on level boundary; worst case ~50K lost on hard power-off
```

50K tokens = 1 level. See [Levels & XP](../concepts/levels-xp.md).

## Mood / fed / energy derivation

See [Mood, Fed, Energy](../concepts/mood-fed-energy.md). Velocity ring drives mood baseline; deny ratio drags it down. Energy starts at 3/5, refills on nap, drains 1 tier per 2h.

## Name sanitization

`_safeCopy` strips `"`, `\`, and control bytes — names go unescaped into `xfer.h`'s status JSON. A stored quote would otherwise break the status endpoint until cleared.

## See Also

- [NVS Layout](../concepts/nvs-layout.md) — all keys centralized
- [Levels & XP](../concepts/levels-xp.md) — token-to-level math
- [Mood, Fed, Energy](../concepts/mood-fed-energy.md)
- [Main / UI](main.md) — load/save call sites
- [Xfer module](xfer.md) — `petNameSet/ownerSet/speciesIdxSave` callers
