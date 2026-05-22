---
title: BLE Bridge Module
type: module
source: src/ble_bridge.{cpp,h}
updated: 2026-05-22
---

# `ble_bridge` — NUS GATT server + bonded link

## Responsibility

Owns the BLE stack: advertises Nordic UART Service, serves the RX/TX characteristics, manages secure pairing, exposes a streaming-stream-ish API (`bleAvailable`/`bleRead`/`bleWrite`) for the rest of the firmware.

## API

```cpp
void     bleInit(const char* deviceName);
bool     bleConnected();
bool     bleSecure();                  // LTK negotiated for current link
uint32_t blePasskey();                 // 6-digit pairing PIN while pending; 0 otherwise
void     bleClearBonds();              // wipe all stored LTKs
size_t   bleAvailable();
int      bleRead();
size_t   bleWrite(const uint8_t* data, size_t len);
```

## Internals

- **RX ring buffer**: 2048 bytes; full → drop (upstream must keep up)
- **MTU**: requests 517; macOS typically negotiates ~185
- **Chunked notify**: `min(mtu-3, 180)` bytes per ATT notify; 4ms delay between chunks lets BLE stack flush
- **Server callbacks**: track `connected`, restart advertising on disconnect, snoop `onMtuChanged`
- **Security callbacks** (`SecCallbacks`):
  - `onPassKeyNotify(pk)` → stores `passkey`, logs to serial
  - `onAuthenticationComplete` → clears passkey, sets `secure`; on failure disconnects
  - DisplayOnly capability (`ESP_IO_CAP_OUT`), key size 16

## Pairing parameters

- Auth: `ESP_LE_AUTH_REQ_SC_MITM_BOND` (Secure Connections + MITM + Bond)
- Encryption level: `ESP_BLE_SEC_ENCRYPT_MITM`
- All NUS characteristics + CCCD: `ESP_GATT_PERM_*_ENCRYPTED`
- Init/resp encryption + ID keys exchanged

## Advertising

- Service UUID added to advertising
- ScanResponse enabled
- ConnInterval: min `0x06`, max `0x12` (iOS-friendly)

## See Also

- [Transport (protocol)](../protocol/transport.md) — UUIDs and frame format
- [Security & Pairing (protocol)](../protocol/security.md) — pairing spec
- [Main / UI](main.md) — calls `bleInit`, polls passkey/secure for screens
