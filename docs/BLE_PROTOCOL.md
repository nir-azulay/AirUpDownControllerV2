# BLE Communication Protocol Documentation (`v1.0` Firmware / `v2.1.0` Apps)

## Device Information
- **Device Name:** Dynamic (configurable in settings, default `esp32-AirUpDownController`)
- **Service UUID:** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Requested MTU Size:** `517` bytes
- **Chunking Window:** `180` bytes bidirectional (for payloads `> 180` bytes)

## Characteristics

### 1. Notification Characteristic (`ESP32 → Mobile`)
- **UUID:** `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- **Properties:** `NOTIFY`
- **CCCD Descriptor:** `0x2902`
- **Description:** Used by the ESP32 controller to push status updates, full settings, authentication results, target-reached events, and Wi-Fi scan results to the mobile client.
- **Chunking:** Payloads exceeding `180` bytes are sent in `180`-byte notification slices with `15 ms` pacing (`notifyBLEPayload()`). Clients reassemble slices until brace depth (`{` and `}`) balances to `0`.

### 2. Command Characteristic (`Mobile → ESP32`)
- **UUID:** `e3223119-9445-4e96-a4a1-85358c4046a2`
- **Properties:** `READ | WRITE`
- **Description:** Used by the mobile client to send JSON commands to the controller.
- **Chunking:** Commands exceeding `180` bytes (such as `command: 3` full settings updates) are split by the mobile client into `180`-byte writes spaced by `25 ms`. The ESP32 buffers chunks in `bleRxBuffer` until a complete JSON object is received.

---

## Device to Client Messages (Notifications)

### 1. Status Update
```json
{
  "Pressure": "28.5",
  "operationMode": "1",
  "pressureValid": true,
  "rawADC": 1420,
  "voltage": 1.144
}
```
- `Pressure`: String-formatted float representing current manifold pressure in PSI (`1` decimal place).
- `operationMode`: String-formatted integer (`"1"` = Stop, `"2"` = Inflate, `"3"` = Deflate).
- `pressureValid`: Boolean indicating whether the sensor signal is within valid operating bounds.
- `rawADC` & `voltage`: Included only when **Debug Mode** (`command: 7`) is enabled.

### 2. Full Settings Update (`operationMode: 4`)
```json
{
  "operationMode": 4,
  "DeviceName": "Raptor-Air-MNG",
  "wifissid": "MyWiFi",
  "wifipassword": "MyPassword",
  "RelayCompressorPin": 17,
  "RelayFreePin": 16,
  "SensorPin": 32,
  "MaxSecForStatus": 30,
  "SensorValueAt0PSI": 0.321,
  "SensorValueAt50PSI": 3.140,
  "SensorFullScalePSI": 50.0,
  "MaxPressurePSI": 50.0,
  "MaxValveOpenSec": 120,
  "AccessPointPassword": "12345678",
  "HasAccessCode": false,
  "MessureWait": 1.00,
  "Precision": 0.30,
  "CurrentPressure": 28.5,
  "Software_VER": "v1.0.261101_256",
  "IP": "192.168.10.155",
  "SimulationMode": false,
  "PressureCalcMethod": 2
}
```
- Sent automatically upon connection (if unlocked) or immediately after successful authentication (`command: 8`), and in response to `command: 6`.
- `HasAccessCode`: `true` if a non-empty `AccessCode` is configured on the controller (`AccessCode` itself is never sent in plaintext over BLE notifications).

### 3. Target Pressure Reached Notification
```json
{
  "type": "target_reached",
  "targetPressure": 32.0,
  "currentPressure": 32.0
}
```
- Sent once when active inflation or deflation reaches `targetPressure` within `±Precision` PSI and the valves close.

### 4. Authentication State Notification
```json
{
  "type": "auth",
  "ok": true,
  "required": false
}
```
- Sent on connection if an `AccessCode` is required (`{"type":"auth","ok":false,"required":true}`) and in response to `command: 8`.

### 5. Wi-Fi Scan Results Notification
```json
{
  "type": "wifi_list",
  "networks": "HomeWiFi|-48;GarageAP|-67;TrailHotspot|-79"
}
```
- Sent in response to `command: 9` (`scan_wifi`). While the ESP32 radio is actively scanning channels, it first notifies `{"type":"wifi_list","scanning":true}` and pushes the semicolon-delimited `SSID|RSSI` list once `WiFi.scanComplete()` finishes.

---

## Client to Device Commands

### 1. Stop Command (`command: 1`)
```json
{
  "command": 1
}
```
- Immediately closes both valves (`StopAir()`) and cancels the active session. Always accepted, even if the connection is not yet authenticated.

### 2. Start / Set Target Pressure Command (`command: 2`)
```json
{
  "command": 2,
  "targetPressure": 32.0
}
```
- `targetPressure`: Desired pressure in PSI (`0 < targetPressure <= min(MaxPressurePSI, SensorFullScalePSI)`).

### 3. Update Settings Command (`command: 3`)
```json
{
  "command": 3,
  "DeviceName": "Raptor-Air-MNG",
  "wifissid": "MyWiFi",
  "wifipassword": "MyPassword",
  "RelayCompressorPin": 17,
  "RelayFreePin": 16,
  "SensorPin": 32,
  "MaxSecForStatus": 30,
  "SensorValueAt0PSI": 0.321,
  "SensorValueAt50PSI": 3.140,
  "SensorFullScalePSI": 50.0,
  "MaxPressurePSI": 50.0,
  "MaxValveOpenSec": 120,
  "AccessPointPassword": "12345678",
  "AccessCode": "",
  "ClearAccessCode": false,
  "MessureWait": 1.0,
  "Precision": 0.3,
  "SimulationMode": false,
  "PressureCalcMethod": 2
}
```
- All fields beyond `"command": 3` are optional; only provided keys are updated.
- Pass `"ClearAccessCode": true` to clear an existing BLE access code, or pass a non-empty `"AccessCode"` to set/change it.
- Settings are validated by `normalizeSettings()`, saved to `/spiffs/settings.json` on the main loop, and the ESP32 reboots after `1.8 s`.

### 4. Simulation Mode Command (`command: 4`)
```json
{
  "command": 4,
  "enabled": true
}
```
- Enables or disables `SimulationMode` (`+0.5 PSI` per step during inflation, `-0.3 PSI` per step during deflation, with physical relays kept `LOW`), persists the setting to flash, and sends an updated settings notification.

### 5. Log Level Command (`command: 5`)
```json
{
  "command": 5,
  "level": 3
}
```
- Sets serial log verbosity (`0` = NONE, `1` = ERROR, `2` = WARNING, `3` = INFO, `4` = DEBUG, `5` = VERBOSE).

### 6. Get Settings Command (`command: 6`)
```json
{
  "command": 6
}
```
- Requests an immediate `operationMode: 4` full settings notification.

### 7. Debug Mode Command (`command: 7`)
```json
{
  "command": 7,
  "enabled": true
}
```
- Enables or disables inclusion of `rawADC` and `voltage` in periodic pressure status notifications.

### 8. Access Code Authentication Command (`command: 8`)
```json
{
  "command": 8,
  "accessCode": "1234"
}
```
- Verifies the access code when `HasAccessCode` is enabled on the controller. On match, replies `{"type":"auth","ok":true,"required":false}` followed by the full settings payload.

### 9. Scan Wi-Fi Networks Command (`command: 9`)
```json
{
  "command": 9,
  "rescan": true
}
```
- Instructs the ESP32 controller to scan nearby 2.4 GHz Wi-Fi networks and return a `wifi_list` notification.
