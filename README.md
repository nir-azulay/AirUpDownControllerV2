# AirUpDown controller

ESP32 firmware that inflates and deflates tires through two 12 V solenoid valves and reads pressure from a sensor on the shared manifold.

- [Hardware, wiring, and air path](docs/HARDWARE.md)
- [Parts to buy](docs/PARTS.md)
- [BLE protocol](docs/BLE_PROTOCOL.md)
- Schematic: `hardware/AirUpDownController.sch`

Build and flash with PlatformIO (`esp32dev`). The configuration page sets calibration, Wi-Fi, and the optional BLE passkey. The onboard relays stay on GPIO16 (outer, inflate) and GPIO17 (deflate), both active high. If an older flash saved pins 25 and 26, set those two fields to 16 and 17 on the configuration page so the firmware drives the pins the relays are already wired to.
