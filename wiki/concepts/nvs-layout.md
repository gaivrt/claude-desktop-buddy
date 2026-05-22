---
title: NVS Layout
type: concept
updated: 2026-05-22
---

# NVS Layout

All persistent state lives in one Preferences namespace: **`"buddy"`**.

## Keys

| Key       | Type    | Default  | Field                                          |
| --------- | ------- | -------- | ---------------------------------------------- |
| `nap`     | UInt    | 0        | cumulative nap seconds                         |
| `appr`    | UShort  | 0        | approvals                                      |
| `deny`    | UShort  | 0        | denials                                        |
| `vel`     | Bytes   | zeroed   | `uint16_t velocity[8]` ring (16 bytes)         |
| `vidx`    | UChar   | 0        | velocity ring write index                      |
| `vcnt`    | UChar   | 0        | velocity ring fill count (≤8)                  |
| `lvl`     | UChar   | 0        | level (derived from tokens; persisted on edge) |
| `tok`     | UInt    | 0        | cumulative output tokens                       |
| `s_snd`   | Bool    | true     | sound on                                       |
| `s_bt`    | Bool    | true     | BT pref (BLE always-on regardless)             |
| `s_wifi`  | Bool    | false    | wifi pref (placeholder, no WiFi stack linked)  |
| `s_led`   | Bool    | true     | LED on attention                               |
| `s_hud`   | Bool    | true     | transcript HUD on NORMAL                       |
| `s_crot`  | UChar   | 0        | clock rotation (0=auto, 1=portrait, 2=landscape) |
| `petname` | String  | `Buddy`  | display name                                   |
| `owner`   | String  | empty    | owner first name                               |
| `species` | UChar   | `0xFF`   | 0..N-1 ASCII species, `0xFF` = GIF mode        |

## Write discipline

NVS sectors have ~100K write cycles. Saves happen only on:

- **Approval** — `statsOnApproval` writes appr + velocity ring
- **Denial** — `statsOnDenial` writes deny
- **Nap end** — `statsOnNapEnd` writes nap
- **Level boundary crossed** — `statsOnBridgeTokens` writes tok + lvl
- **Setting toggled** — `applySetting` → `settingsSave`
- **Name/owner/species change** — direct putString/putUChar

**Never on timer.** Heartbeat token deltas accumulate in RAM only — worst case ~50K tokens lost on hard power-off.

## Token tracking nuance

`statsOnBridgeTokens` has a first-sight latch (`_tokensSynced`) so a device reboot doesn't re-credit the entire bridge session. See [stats.md](../firmware/stats.md#token-tracking--bridge-restart-vs-device-reboot).

## Factory reset

`_prefs.begin("buddy", false); _prefs.clear(); _prefs.end();` plus `LittleFS.format()` + `bleClearBonds()`. Triggered from settings → reset → factory reset → tap twice within 3s.

## Name sanitization

`petname` and `owner` go unescaped into `xfer.h` status JSON. `_safeCopy` strips `"`, `\`, and control bytes before persisting — otherwise a stored quote would break the status endpoint until the name was reset.

## See Also

- [Stats module](../firmware/stats.md)
- [Levels & XP](levels-xp.md)
- [Main / UI](../firmware/main.md) — factory reset implementation
