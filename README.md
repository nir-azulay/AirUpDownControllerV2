# AirUpDown controller

ESP32 firmware that inflates and deflates tires through two 12 V solenoid valves and reads pressure from a sensor on the shared manifold.

- [Hardware, wiring, and air path](docs/HARDWARE.md)
- [Parts to buy](docs/PARTS.md)
- [BLE protocol](docs/BLE_PROTOCOL.md)
- Schematic: `hardware/AirUpDownController.sch`

Build and flash with PlatformIO (`esp32dev`). The onboard relays stay on GPIO16 (outer, inflate) and GPIO17 (deflate), both active high. The pressure sensor uses GPIO34. If an older flash saved relay pins 25 and 26, or sensor pin 27, set those fields on the configuration page. Enter the PSI stamped on the sensor there as well. Both valves stay closed until that full-scale value is set.

The BLE messages keep the same fields the phone apps already use. Maximum pressure, valve-open time, sensor full scale, and the access-point password are configuration-page settings only.
