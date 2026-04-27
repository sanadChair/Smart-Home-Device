# ESP8266 Smart Home Device Firmware

Binary-state smart home controller for the ESP-01 (ESP8266).  
Self-provisioning via captive portal · UDP discovery · REST control.

---

## Hardware

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 0 | Setup trigger button | Active LOW, internal pullup. **Boot-critical** — hold LOW at power-on = flash mode. |
| GPIO 2 | Relay / LED output | Active LOW. Mirrors device state. |
| GPIO 1 (TX) | UART TX | Leave connected during development. |
| GPIO 3 (RX) | UART RX | Leave connected during development. |

---

## Flashing

1. Install [PlatformIO](https://platformio.org/).
2. Open the project folder.
3. **Edit `src/device_state.h`** — change `DEVICE_ID_CONSTANT` to a unique ID for this unit:
   ```c
   #define DEVICE_ID_CONSTANT "relay-ab9f3c2d"
   ```
4. Wire your USB-TTL adapter, hold GPIO 0 LOW, cycle power to enter flash mode.
5. Run:
   ```
   pio run --target upload
   pio run --target uploadfs   # uploads LittleFS partition
   ```

---

## First Boot — Provisioning

1. Power on (GPIO 0 floating/HIGH — normal boot).
2. Device finds no stored SSID → starts AP: `SmartDevice-relay` (or similar).
3. Connect your phone/laptop to that open Wi-Fi network.
4. Browse to `http://192.168.4.1/`.
5. Enter a device name, your home SSID, and password → click **Connect**.
6. Device reboots into Operational Mode and joins your network.

---

## Entering Setup Mode (Re-provisioning)

Hold GPIO 0 LOW for **10 continuous seconds** at any time during normal operation.  
The device erases stored credentials and restarts into the captive portal.

---

## REST API (Operational Mode)

### `GET /status?deviceId=<id>`

Returns current state.

```json
{
  "deviceId": "relay-ct2112rx1",
  "name": "Living Room Relay",
  "controlType": "binary",
  "state": 0
}
```

### `POST /control`

Change state.

**Request body:**
```json
{ "deviceId": "relay-ct2112rx1", "state": 1 }
```

**Response:** same shape as `/status`, `state` reflecting new value.

**Error codes:** `400` invalid body/state · `403` deviceId mismatch.

---

## UDP Discovery

Send **any** UDP packet to port `9988` (broadcast or unicast) from the same network.  
Each device replies to the sender with its JSON descriptor.

```bash
# Quick test from Linux/macOS:
echo '{"type":"discovery", "requestId": "1"}'| nc -u -w1 255.255.255.255 9988
```

---

## Module Map

```
src/
├── main.cpp            Boot sequence, mode dispatch, main loop
├── device_state.h/cpp  In-memory state, GPIO helpers, deviceId constant
├── storage.h/cpp       LittleFS JSON persistence
├── setup_mode.h/cpp    AP hotspot + captive config portal
├── wifi_manager.h/cpp  STA connection, retry logic, reconnect
├── udp_listener.h/cpp  UDP port 9988 discovery responder
├── http_server.h/cpp   GET /status, POST /control
└── button_handler.h/cpp GPIO 0 debounce + 10 s hold detector
```

---

## Dependencies

| Library | Version |
|---------|---------|
| ESP8266WiFi | bundled with ESP8266 Arduino core |
| ESP8266WebServer | bundled |
| WiFiUdp | bundled |
| LittleFS | bundled |
| ArduinoJson | ^6.21.5 (auto-fetched by PlatformIO) |
