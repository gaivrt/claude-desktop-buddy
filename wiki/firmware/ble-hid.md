---
title: BLE HID Module (keyboard)
type: module
source: src/ble_hid.{cpp,h}
updated: 2026-05-23
---

# `ble_hid` — BLE HID keyboard attached to the shared server

## Responsibility

Adds a BLE HID keyboard (plus DIS + Battery services) to the **same** `BLEServer` the [NUS bridge](ble-bridge.md) creates. macOS / Windows Bluetooth settings pair the device as a regular keyboard; the Claude desktop app still talks NUS over the same connection. Drives the [Voice screen](../concepts/screens.md#voice) ASR hotkey.

This is a **fork-specific extension**, not part of the published BLE protocol. See [protocol-vs-implementation](../decisions/protocol-vs-implementation.md#non-spec-extensions).

## API

```cpp
void hidInit(BLEServer* server);                          // attach HID/DIS/Battery
bool hidConnected();                                      // BLE link up AND host subscribed to input CCCD
void hidSendKey(uint8_t modifier, uint8_t keycode);       // press + 10ms + release
void hidSendEnter();                                      // convenience for 0x28
void hidTick();                                           // call from loop() — 60s battery refresh
```

## Init ordering (critical)

Driven by `main.cpp::startBt`:

```cpp
bleInit(btName);                  // creates server, NUS service, security, enrolls NUS UUID in adv
hidInit(bleGetServer());          // attaches HID/DIS/Battery, enrolls HID UUID + Appearance(Keyboard)
bleStartAdvertising();            // single advertisement carries BOTH UUIDs
```

If you start advertising before `hidInit`, the HID UUID never makes it into the advertising record and the OS won't classify the device as a keyboard during pairing. See [CP2 in the plan](#) for the split.

## HID report descriptor

Standard 8-byte **boot keyboard** report (USB HID spec, Keyboard Boot Interface):

| Byte | Meaning                                                              |
| ---- | -------------------------------------------------------------------- |
| 0    | Modifier bitmap: LCtrl=0x01, LShift=0x02, LAlt=0x04, **LGUI=0x08**, RCtrl=0x10, RShift=0x20, RAlt=0x40, RGUI=0x80 |
| 1    | Reserved (always 0)                                                  |
| 2-7  | Up to six concurrently-pressed HID usage codes from the Keyboard page |

Usages used by this firmware:
- `0xE7` — **Right GUI** / Right Cmd (mac ASR hotkey)
- `0x08 | 0x0B` — Left GUI modifier + `H` keycode (**Win+H**, Windows dictation)
- `0x28` — **Enter** (submit dictation)

## Security

Every HID characteristic and its CCCD has `ESP_GATT_PERM_*_ENCRYPTED` set via `_encryptChar()`. This **must** match the NUS bridge's `SC_MITM_BOND` posture — mixed permissions across services on the same advertisement break Windows HID auto-attach and weaken macOS bonding. See `.review/log.md` (c1 pattern).

## DIS strings

| Field         | Value                          |
| ------------- | ------------------------------ |
| Manufacturer  | `"Anthropic Maker"`            |
| PNP VID source | 0x02 (USB)                    |
| PNP VID/PID/rev | 0x05AC / 0x820A / 0x0100     |

PNP VID 0x05AC = Apple — this is a placeholder for development; a fork shipping to others should pick its own VID (vendors can register at usb.org/getting-vendor-id) or use 0xFFFF for non-listed.

## Appearance

`BLEDevice::setAppearance(0x03C1)` — Keyboard. BLEHIDDevice's constructor sets generic HID (960); this overrides to Keyboard so Windows picks the keyboard icon during discovery + HID class auto-attach.

## `hidConnected()` honesty

Checks both BLE link **and** the input-report CCCD's `getNotifications()`. Without the CCCD check, the Voice screen would say "linked" while notifies silently drop. See `.review/log.md` (c1 pattern).

## `hidSendKey` constraint

Contains a 10ms `delay()` between press and release. **Must be called from `loop()` context**, not a BLE/GATT callback — a stalled BLE task risks a supervision-timeout disconnect. The firmware's only caller is the main-loop button handler, which is safe.

## Battery refresh

`hidTick()` polls `M5.Axp.GetBatVoltage()` and pushes via `_hid->setBatteryLevel()` with a 60s internal throttle. Caller invokes from `loop()` next to `clockRefreshRtc()`.

## See Also

- [BLE Bridge module](ble-bridge.md) — owns the BLEServer; HID attaches to it
- [Main / UI](main.md) — `startBt` ordering + Voice page button handling
- [ASR Integration](../concepts/asr-integration.md) — hotkey choice + dual-mode mac/win
- [Voice Screen](../concepts/screens.md#voice)
- [Security (protocol)](../protocol/security.md) — NUS pairing posture HID must match
- [Protocol vs Implementation](../decisions/protocol-vs-implementation.md) — HID listed as fork-specific extension
