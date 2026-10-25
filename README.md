# AirUpDown controller

ESP32 firmware that inflates and deflates tires through two 12 V solenoid valves and reads pressure from a sensor on the shared manifold.

- [Hardware, wiring, air path, and parts](docs/HARDWARE.md)
- [BLE protocol](docs/BLE_PROTOCOL.md)
- Schematic: `hardware/AirUpDownController.sch`

Build and flash with PlatformIO (`esp32dev`). The configuration page sets the relay pins, sensor pin, calibration, Wi-Fi, and optional BLE passkey. Relay defaults are GPIO16 and GPIO17, both active high. A device that already saved older pins keeps those saved values until they are changed on that page.
