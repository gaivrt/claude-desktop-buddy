# Review Log

Cross-checkpoint pattern memory for gan-hid-asr team. Reviewer reads this file before each review; leader appends after blocking verdicts.

## [2026-05-23] hid-asr/c1 | REVISE→PASS
Pattern: when adding new BLE characteristics to a device that already pairs SC+MITM+Bond, every new char + its CCCD must `setAccessPermissions(ESP_GATT_PERM_*_ENCRYPTED)` to match — mixed permissions across services break Windows HID auto-attach and weaken macOS bonding.
Context: src/ble_hid.cpp:_encryptChar; mirrors ble_bridge.cpp:106-115 (NUS TX/RX/CCCD encrypted).

Pattern: "connected" status on a notify characteristic must check the CCCD's `getNotifications()`, not just BLE link state — otherwise UI lies and notifies silently drop.
Context: src/ble_hid.cpp:hidConnected; relevant for any future HID/notify status indicator in main.cpp.

Pattern: any HID send API that does press → delay → release MUST be documented as loop()-only (BLE callback context would stall the GATT stack).
Context: src/ble_hid.cpp:hidSendKey comment.

## [2026-05-23] hid-asr/c5 | REVISE→PASS
Pattern: when adding release-based or long-press button paths to a button that already participates in the wake-swallow mechanism, the new paths MUST also honor the swallow flag, and the flag MUST be cleared at end of `wasReleased` (not inside `wasPressed`) so the full press-release cycle is consumed. Mirror the existing BtnA pattern (swallowBtnA cleared at end of `wasReleased`).
Context: src/main.cpp BtnB Voice block; same pattern applies if anyone later adds long-press to other buttons.


