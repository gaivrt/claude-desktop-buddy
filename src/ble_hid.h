#pragma once
#include <stdint.h>

// BLE HID keyboard service attached to the same BLEServer used by the NUS
// bridge. macOS / Windows Bluetooth settings pair us as a regular keyboard;
// the desktop app still talks NUS over the same connection.
//
// Standard 8-byte boot keyboard report:
//   byte 0  modifier bitmap (LCtrl=0x01, LShift=0x02, LAlt=0x04, LGUI=0x08,
//                            RCtrl=0x10, RShift=0x20, RAlt=0x40, RGUI=0x80)
//   byte 1  reserved (always 0)
//   bytes 2-7  up to six concurrently-pressed HID usage codes

class BLEServer;

// Adds HID + DIS + Battery services to the shared BLEServer. Call AFTER
// bleInit() and BEFORE bleStartAdvertising() — advertising must include the
// HID UUID so the OS recognizes us as a keyboard during pairing.
void hidInit(BLEServer* server);

// True once an HID central has subscribed to the input report notifications.
// Until then, hidSendKey() is a no-op.
bool hidConnected();

// Press one (modifier, keycode) pair, then release. modifier is the
// bitmap from byte 0 above; keycode is a single HID usage from the
// keyboard page (e.g. 0x0B = H, 0x28 = Enter, 0xE7 = Right GUI).
void hidSendKey(uint8_t modifier, uint8_t keycode);

// Convenience: press + release Enter (0x28, no modifier).
void hidSendEnter();

// Call from the main loop() at 1Hz-ish. Refreshes the battery characteristic
// so the OS battery indicator stays current even when no key is pressed.
// Cheap (one AXP read with an internal 60s throttle).
void hidTick();
