---
title: ASR Integration (Voice page hotkeys)
type: concept
updated: 2026-05-23
---

# ASR Integration

The Voice page turns Buddy into a one-button trigger for the host OS's dictation tool. Default behavior:

| `settings.hostOs` | Short B (release)         | Long B (≥500ms)  | Use case                                              |
| ----------------- | ------------------------- | ---------------- | ----------------------------------------------------- |
| `0` (mac)         | Right Cmd (`0xE7`)        | Enter (`0x28`)   | SuperWhisper / WhisprFlow default hotkey              |
| `1` (win)         | Win+H (`0x08`, `0x0B`)    | Enter (`0x28`)   | Windows 10/11 built-in dictation (Win+H)              |

Default is **win** for the Windows fork target.

## Why these hotkeys

**macOS — Right Cmd alone**:
SuperWhisper and WhisprFlow let the user bind a custom global hotkey. Right Cmd by itself is a popular choice because it's never used by the OS as a modifier-only sequence — pressing it alone is effectively a no-op for the system, ideal for ASR tools.

**Windows — Win+H**:
Built into Windows 10/11. Toggles the dictation overlay in any text input. Zero third-party install required. Works in browsers, native apps, and even in the Windows search box. The downside is the overlay is modal — clicking elsewhere closes it — but that suits a "press-talk-release-submit" workflow.

**Long press → Enter (both modes)**:
After dictation transcribes your speech into the text field, Enter submits the form / sends the message. The 500ms threshold is short enough to feel snappy but long enough that an accidental press during a deliberate short tap doesn't fire it.

## How it gets routed

[`main.cpp` B-button block](../firmware/main.md):

1. `inVoice = displayMode == DISP_VOICE && !menus && !inPrompt` precondition
2. `pressedFor(500)` edge → set `btnBLong`, send Enter via `hidSendEnter()`
3. `wasPressed` in Voice → no-op (defer to release)
4. `wasReleased` in Voice + `!btnBLong` + `!swallowBtnB` → branch on `hostOs` and send short key

The `!swallowBtnB` guard is critical: when the screen is off and user wakes via B, the press+release must NOT fire a spurious dictation hotkey. See `.review/log.md` (c5 pattern).

## How keys leave the device

[`ble_hid` module](../firmware/ble-hid.md) `hidSendKey(modifier, keycode)`:
1. Fill 8-byte boot keyboard report
2. `inputChar->notify()` (press)
3. 10ms `delay()`
4. Zero report
5. `inputChar->notify()` (release)

OS HID stack delivers a standard key event to the focused window — indistinguishable from a real keyboard.

## Scope guard

HID services are always live; the OS keeps the keyboard pairing across screens. But Buddy **only routes B → HID when on the Voice page**. On NORMAL / PET / INFO / Approval / Clock / menus, B does its original thing (deny / page / scroll / nav). This avoids accidentally toggling dictation while reading transcript.

## Pairing flow

1. Settings → Bluetooth → Add device → "Claude-XXXX"
2. Buddy displays 6-digit passkey via [`drawPasskey()`](../firmware/main.md) (same path NUS pairing uses)
3. User types it in the OS pairing dialog
4. AES-CCM-encrypted from then on
5. Reconnect across reboots is automatic (LTK in NVS)

If the user clicks "Forget" in OS Bluetooth, the OS sends `cmd:"unpair"` over NUS → `bleClearBonds()` wipes the stored LTK. Next pairing shows a fresh passkey.

## Not-paired UX

`drawVoice` shows "pair in OS settings" in dim text when `hidConnected() == false`. Pressing B in this state does nothing — no beep, no key send. Once paired and host subscribes to the input-report CCCD, the same render shows "linked" in green.

## Magic numbers

| Value     | Meaning                                                        |
| --------- | -------------------------------------------------------------- |
| `500ms`   | Long-press threshold for Enter                                 |
| `10ms`    | press → release delay inside `hidSendKey`                      |
| `60000ms` | Battery characteristic refresh throttle in `hidTick`           |
| `0xE7`    | HID usage: Right GUI (Right Cmd)                               |
| `0x08`    | HID modifier bit: Left GUI                                     |
| `0x0B`    | HID usage: `H` keycode                                         |
| `0x28`    | HID usage: Enter                                               |
| `0x03C1`  | BLE Appearance: Keyboard                                       |

## See Also

- [BLE HID module](../firmware/ble-hid.md)
- [Voice Screen](screens.md#voice)
- [Main / UI](../firmware/main.md) — button state machine
- [NVS Layout](nvs-layout.md) — `s_host` key
- [Protocol vs Implementation](../decisions/protocol-vs-implementation.md)
