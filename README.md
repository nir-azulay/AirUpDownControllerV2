# AirUpDown Controller (`v1.0`)

ESP32 firmware that inflates and deflates tires through two 12 V solenoid valves and reads pressure from a sensor on the shared manifold.

- [Hardware, wiring, and air path](docs/HARDWARE.md)
- [Parts to buy](docs/PARTS.md)
- [BLE protocol](docs/BLE_PROTOCOL.md)
- Schematic: `hardware/AirUpDownController.sch`

Build and flash with PlatformIO (`esp32dev`). By default, the onboard relays use GPIO16 (inflate) and GPIO17 (deflate), both active high, and the pressure sensor uses GPIO34 (ADC1). Boards with different wiring (such as GPIO17, GPIO16, and GPIO32) can configure their pins on the Web UI (`/configurations`) or from the iOS and Android apps over BLE.

All 18 controller settings—including GPIO pins, ADC or voltage-divider calibration, sensor full-scale PSI, maximum pressure, maximum valve-open time, Wi-Fi SSID (with built-in 2.4 GHz network scanner), hotspot password, and BLE access code—can be configured from both the built-in Web UI and the mobile apps over BLE.
