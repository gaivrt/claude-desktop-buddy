#include "ble_hid.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>
#include <BLECharacteristic.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>
#include <Arduino.h>
#include <M5StickCPlus.h>

static BLEHIDDevice*      _hid     = nullptr;
static BLECharacteristic* _input   = nullptr;
static uint32_t           _lastBat = 0;

// Standard HID boot-keyboard report descriptor. Lifted from the
// USB HID spec section "Keyboard Boot Interface" so any OS that
// supports BLE keyboards (macOS, Win10+, Linux) accepts it.
static const uint8_t REPORT_MAP[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x06,        // Usage (Keyboard)
  0xA1, 0x01,        // Collection (Application)
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0xE0,        //   Usage Minimum (224, LCtrl)
  0x29, 0xE7,        //   Usage Maximum (231, RGUI)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data, Variable, Absolute) — modifiers
  0x95, 0x01,        //   Report Count (1)
  0x75, 0x08,        //   Report Size (8)
  0x81, 0x01,        //   Input (Constant) — reserved
  0x95, 0x06,        //   Report Count (6)
  0x75, 0x08,        //   Report Size (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x65,        //   Logical Maximum (101)
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0x00,        //   Usage Minimum (0)
  0x29, 0x65,        //   Usage Maximum (101)
  0x81, 0x00,        //   Input (Data, Array) — six keycodes
  0xC0               // End Collection
};

class HidOutCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic*) override {
    // Hosts write here to set LED state (CapsLock etc). We don't have LEDs to
    // mirror, but accept the write so the host doesn't retry.
  }
};

// Lock every HID characteristic and its CCCD to encrypted-only so the
// pairing posture matches the NUS bridge's SC+MITM+Bond setup. Without this
// the OS would mount HID over a weaker (or unencrypted) channel than NUS
// — Windows in particular refuses to attach as a keyboard when the
// permissions disagree across services on the same device.
static void _encryptChar(BLECharacteristic* c, bool read, bool write) {
  if (!c) return;
  esp_gatt_perm_t perms = (esp_gatt_perm_t)0;
  if (read)  perms = (esp_gatt_perm_t)(perms | ESP_GATT_PERM_READ_ENCRYPTED);
  if (write) perms = (esp_gatt_perm_t)(perms | ESP_GATT_PERM_WRITE_ENCRYPTED);
  c->setAccessPermissions(perms);
  BLE2902* cccd = (BLE2902*)c->getDescriptorByUUID((uint16_t)0x2902);
  if (cccd) {
    cccd->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  }
}

static void _refreshBattery() {
  if (!_hid) return;
  int vBat = (int)(M5.Axp.GetBatVoltage() * 1000);
  int pct = (vBat - 3200) / 10;
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  _hid->setBatteryLevel((uint8_t)pct);
}

void hidInit(BLEServer* server) {
  if (!server || _hid) return;
  _hid = new BLEHIDDevice(server);

  // Device Information Service strings — what shows in the OS pairing dialog
  // and the bluetooth settings page.
  _hid->manufacturer()->setValue("Anthropic Maker");
  _hid->pnp(0x02, 0x05AC, 0x820A, 0x0100);   // 0x02 = USB VID source; placeholder VID/PID/rev
  _hid->hidInfo(0x00, 0x01);                  // bCountryCode=0, flags=remote-wake

  _input = _hid->inputReport(1);
  BLECharacteristic* output = _hid->outputReport(1);
  output->setCallbacks(new HidOutCallbacks());

  // Match NUS encryption posture across HID input + output + their CCCDs.
  _encryptChar(_input,  true,  false);
  _encryptChar(output,  true,  true);

  _hid->reportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));
  _hid->startServices();

  // Register HID service + Keyboard appearance in the advertisement so the
  // OS recognizes us as a keyboard at scan time. Actual startAdvertising()
  // is deferred to bleStartAdvertising() — this only enrolls the data.
  // Note: ESP32 Arduino BLE 2.0.0 exposes setAppearance on BLEAdvertising,
  // not BLEDevice. BLEHIDDevice's ctor sets generic HID (960); we override
  // here to Keyboard (961 = 0x03C1) so Windows picks the keyboard icon +
  // HID class auto-attach.
  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->setAppearance(0x03C1);
  adv->addServiceUUID(_hid->hidService()->getUUID());

  _refreshBattery();
  _lastBat = millis();
  Serial.println("[hid] keyboard service up");
}

bool hidConnected() {
  // Honest answer: BLE link up AND the host has subscribed to the input
  // report CCCD (otherwise notifies are dropped silently). Without the CCCD
  // check the UI would claim "linked" while keypresses go nowhere.
  extern bool bleConnected();
  if (!bleConnected() || !_input) return false;
  BLE2902* cccd = (BLE2902*)_input->getDescriptorByUUID((uint16_t)0x2902);
  return cccd && cccd->getNotifications();
}

static void _sendReport(uint8_t modifier, uint8_t keycode) {
  if (!_input) return;
  uint8_t report[8] = { modifier, 0, keycode, 0, 0, 0, 0, 0 };
  _input->setValue(report, sizeof(report));
  _input->notify();
}

// IMPORTANT: must be called from loop() context, NOT from a BLE/GATT callback.
// The 10ms delay between press and release would stall the BLE stack and risk
// a supervision-timeout disconnect. In this firmware all callers are the
// main-loop button handler.
void hidSendKey(uint8_t modifier, uint8_t keycode) {
  if (!_hid) return;
  _sendReport(modifier, keycode);
  delay(10);
  _sendReport(0, 0);   // release
}

void hidSendEnter() {
  hidSendKey(0, 0x28);
}

void hidTick() {
  uint32_t now = millis();
  if (now - _lastBat < 60000) return;
  _lastBat = now;
  _refreshBattery();
}
