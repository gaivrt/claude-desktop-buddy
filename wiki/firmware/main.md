---
title: Main Module (loop, state machine, UI)
type: module
source: src/main.cpp
updated: 2026-05-22
---

# `main.cpp` — the loop, persona state machine, UI screens

45KB. Entry point + everything that doesn't fit cleanly into another module: persona derivation, button handling, sleep/wake, menu/settings/reset overlays, the six info pages and two pet pages, transcript HUD, clock face, approval panel, passkey display.

## Globals & key state

| Variable            | Meaning                                                                 |
| ------------------- | ----------------------------------------------------------------------- |
| `TamaState tama`    | mirrored from `dataPoll()`                                              |
| `baseState`         | derived from tama (idle/busy/attention/celebrate)                       |
| `activeState`       | actually rendered; `triggerOneShot` overrides for N ms                  |
| `oneShotUntil`      | clears one-shot                                                         |
| `displayMode`       | NORMAL / PET / INFO / VOICE (cycled by A; 4 modes)                      |
| `btnBLong`          | *(removed)* — was the Win+H long-press→Enter latch (HID gone)            |
| `infoPage` / `petPage` | sub-pages within INFO/PET (cycled by B)                              |
| `menuOpen`, `settingsOpen`, `resetOpen` | modal stack                                       |
| `buddyMode`, `gifAvailable` | ASCII vs GIF                                                    |
| `screenOff`, `dimmed`, `napping` | power state                                                |
| `clockOrient`       | 0=portrait, 1/3=landscape (gravity-detected)                            |

## Persona derivation

```cpp
PersonaState derive(const TamaState& s) {
  if (!s.connected)            return P_IDLE;
  if (s.sessionsWaiting > 0)   return P_ATTENTION;
  if (s.recentlyCompleted)     return P_CELEBRATE;
  if (s.sessionsRunning >= 3)  return P_BUSY;
  return P_IDLE;
}
```

One-shot states (override base for N ms):
- `P_HEART` — approval response in under 5s (2s)
- `P_DIZZY` — shake detected (2s)
- `P_CELEBRATE` — level-up event from `statsPollLevelUp()` (3s)

## Loop responsibilities (~16ms cycle)

1. `M5.update()` + `M5.Beep.update()`
2. `dataPoll(&tama)` — pulls USB + BLE bytes
3. `statsPollLevelUp()` → one-shot celebrate
4. `derive()` → `baseState`
5. Wake-transition hold: keep `P_SLEEP` 12s after wake unless urgent state
6. `oneShotUntil` check → final `activeState`
7. LED pulse if attention + `settings().led`
8. Shake check (50ms throttle) → dizzy one-shot
9. Prompt arrival detection → beep + force NORMAL + close overlays
10. Button handling (A short / A long 600ms / B / Power short / Power long 6s)
11. Clock-face decision: USB + RTC-valid + no activity + no overlay → enter clock
12. Render sprite OR direct-to-LCD landscape clock
13. Face-down nap state machine (hysteresis: 15 frames in, -8 out)
14. Screen-off idle timer (30s) — disabled on USB power
15. `delay(screenOff ? 100 : 16)`

## Boot sequence (`setup`)

1. `M5.begin/Imu.Init/Beep.begin`
2. `startBt()` — `bleInit` → `bleStartAdvertising()`; advertises as `Claude-XXXX` (last 2 MAC bytes), **NUS only** (`hidInit` removed — it broke Claude desktop on Windows; see [Voice ASR](../decisions/voice-asr.md)). `setup()` also calls `micInit()` for the Voice screen
3. `statsLoad/settingsLoad/petNameLoad/buddyInit`
4. `characterInit(nullptr)` — scans for first installed character
5. Determine `buddyMode` from `gifAvailable` + `speciesIdxLoad()`
6. Greeting splash for 1.8s: `"<Owner>'s <Pet>"` or `"Hello! a buddy appears"`

## Menus

| Menu         | Items                                                                              |
| ------------ | ---------------------------------------------------------------------------------- |
| **menu**     | settings, turn off, help, about, demo (toggle), close                              |
| **settings** | brightness, sound, bluetooth, wifi, led, transcript, clock rot, host os, ascii pet, reset, back |
| **reset**    | delete char, factory reset, back — tap-twice (3s arm) confirms                     |

Help → INFO page 1 (buttons), About → INFO page 5 (credits).

## Reset semantics

- **Delete char** — wipe `/characters/`, reboot → ASCII mode
- **Factory reset** — `_prefs.clear()` NVS namespace + `LittleFS.format()` + `bleClearBonds()`, reboot

## Screens

| Screen     | Drawn by          | Trigger                                  |
| ---------- | ----------------- | ---------------------------------------- |
| Approval   | `drawApproval`    | `tama.promptId[0]` (within HUD region)   |
| Transcript | `drawHUD`         | NORMAL mode, no prompt, `settings.hud`   |
| Info       | `drawInfo`        | DISP_INFO (6 pages)                      |
| Pet        | `drawPet`         | DISP_PET (2 pages: stats, how-to)        |
| Voice      | `drawVoice`       | DISP_VOICE — Buddy-mic Chinese dictation (hold B); see [Voice Capture](voice-capture.md) |
| Clock      | `drawClock`       | USB + idle + RTC valid (portrait/landscape) |
| Passkey    | `drawPasskey`     | `blePasskey() != 0`                      |
| Menu/settings/reset | `drawMenu` / `drawSettings` / `drawReset` | overlay                  |

Details in [screens.md](../concepts/screens.md).

## Pet cycling (`nextPet`)

GIF (if installed) → ASCII species 0 → 1 → ... → N-1 → GIF. Persisted to `species` NVS key; `0xFF` sentinel = GIF mode.

## Hardware mappings

- **A button** → `M5.BtnA` (front)
- **B button** → `M5.BtnB` (right)
- **Power button** → `M5.Axp.GetBtnPress()` (`0x02` = short, hardware-managed 6s long-press powers off)
- **LED** → GPIO 10, active-low; pulses every 400ms during `P_ATTENTION`
- **Buzzer** → `M5.Beep.tone(freq, durMs)`; gated by `settings.sound`
- **IMU** → `M5.Imu.getAccelData(ax, ay, az)` for shake + face-down + clock orientation
- **RTC** → `M5.Rtc.SetTime/GetTime` (PCF8563 over I2C; shares bus with IMU — cached at 1Hz)
- **Battery** → `M5.Axp.GetBatVoltage/Current/VBusVoltage`
- **Screen backlight** → `M5.Axp.ScreenBreath(20..100)`; `SetLDO2(false)` powers display off

## See Also

- [Screens](../concepts/screens.md)
- [Seven States](../concepts/seven-states.md)
- [Clock Face](../concepts/clock-face.md)
- [Mood / Fed / Energy](../concepts/mood-fed-energy.md)
- [Levels & XP](../concepts/levels-xp.md)
- [Voice Capture](voice-capture.md) — Voice screen mic capture (方案 B, current)
- [ASR Integration](../concepts/asr-integration.md) — old Win+H/HID routing (removed)
- [Data module](data.md), [Stats module](stats.md), [BLE Bridge](ble-bridge.md), [BLE HID](ble-hid.md), [Character](character.md), [Buddy](buddy.md), [Xfer](xfer.md)
