---
title: Security & Pairing
type: protocol-msg
source: REFERENCE.md
updated: 2026-05-22
---

# Security & Pairing

The desktop app connects whether or not your device requests encryption, but **transcript snippets and tool-call hints flow over this link**, so an unencrypted device is sniffable by anyone in radio range with a cheap nRF dongle.

## Recommendation

Require **LE Secure Connections bonding**:
- Mark NUS characteristics (and the TX CCCD) as encrypted-only
- Advertise **DisplayOnly** IO capability
- First GATT access triggers OS pairing → desktop prompts for the 6-digit passkey your device displays → AES-CCM-encrypted from then on
- Reconnects reuse the stored LTK without re-prompting

## Protocol hooks

| Hook                | Purpose                                                                                |
| ------------------- | -------------------------------------------------------------------------------------- |
| `data.sec` in status | `true` once the link is encrypted (or `false`/omit if you don't bond)                  |
| `{"cmd":"unpair"}`  | Desktop sends when user clicks **Forget** → erase your bonds → next pairing fresh passkey |

## Reference impl (ble_bridge.cpp)

- `BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM)`
- Auth mode: `ESP_LE_AUTH_REQ_SC_MITM_BOND` (Secure Connections + MITM + Bond)
- IO capability: `ESP_IO_CAP_OUT` (DisplayOnly)
- NUS RX/TX characteristics get `ESP_GATT_PERM_*_ENCRYPTED`; CCCD too
- `SecCallbacks::onPassKeyNotify` stores `passkey`; `blePasskey()` exposed for `drawPasskey()` UI
- `onAuthenticationComplete`: clears passkey, sets `secure = cmpl.success`; on failure, disconnects
- `bleClearBonds()` enumerates `esp_ble_get_bond_device_list` and calls `esp_ble_remove_bond_device` for each

## See Also

- [Transport (NUS)](transport.md) — the characteristics being protected
- [BLE Bridge module](../firmware/ble-bridge.md) — implementation
- [Commands & Acks](commands-acks.md) — `unpair` command, `sec` in status response
