# AirUpDown controller hardware

The controller inflates and deflates tires from one shared air volume. An ESP32 board with two onboard relays drives two 12 V solenoid valves. A pressure sensor on the same volume reports tire pressure.

The schematic is `hardware/AirUpDownController.sch`. BLE messages are described in [BLE_PROTOCOL.md](BLE_PROTOCOL.md). The phone apps live in the sibling project `TiresAirPressureManagement`.

## Air path

The manifold is one shared volume. Every port on it sees the same pressure.

- The air tank is the inflate source only. It feeds the inflate valve (Y1).
- When Y1 opens, tank air fills the manifold and all of the tires.
- The deflate valve (Y2) takes air from the manifold and vents it to atmosphere.
- With both valves closed, the manifold holds pressure.
- The pressure sensor (U3) is teed into that same manifold, so it reads tire pressure.

Each solenoid is one part: the 12 V coil and the air valve. The valve air ports are NPT and the manifold is 8 mm push-fit, so each joint between them needs a short adapter. The sensor is 1/8 NPT male and needs an 8 mm push-fit adapter into the manifold.

Order a manifold with at least 6 ways: inflate, deflate, sensor, and four tires. An 8-way block leaves spare ports.

## Power and relays

Use a 12 V supply of at least 2 A. Put a 2 A fuse on the 12 V feed before the board.

Connect 12 V to the board terminal marked **VCC**. Do not put 12 V on the pin marked **5 V**. That pin is the regulator output and powers the sensor.

Both relay commons get the fused 12 V. The normally closed contacts are unused.

| Relay | GPIO | Active level | Load |
|---|---|---|---|
| Relay 1, outer | 16 | High = on | Inflate valve Y1 |
| Relay 2 | 17 | High = on | Deflate valve Y2 |

The other coil wire of each valve returns to supply ground, together with the board GND pin. Only one valve runs at a time. Firmware holds both relay pins low at boot and whenever air is stopped.

Put a 1N4007 across each coil. The band points toward the relay NO side, which is the positive side of the coil.

A board that was flashed before these pin defaults still has its saved settings. On the configuration page set the compressor pin to 16 and the free pin to 17, then save. Flashing alone does not replace values already stored on the device.

## Pressure sensor

The transducer output is 0.5 V at zero pressure and 4.5 V at the full-scale PSI stamped on the hex. Do not connect the green wire straight to an ESP32 pin.

| Sensor wire | Goes to |
|---|---|
| Red | Board 5 V output |
| Black | Board GND, which is the same ground as the 12 V supply negative |
| Green | R5, then the divider into the sensor GPIO |

R5 and R6 are 10 kΩ. R5 is in series with the green wire. R6 goes from the divider output to ground. C3 is 100 nF from that same output to ground. The divider presents about 0.25 V to 2.25 V to the pin.

The default sensor pin is GPIO27. That pin is an ADC2 input and cannot be read reliably while Wi-Fi is in use. GPIO34 is an ADC1 input on the header and is a better choice if the reading drops out with Wi-Fi connected. The firmware default is still GPIO27.

## Calibration

The configuration page has two calculation methods.

- Method 1, the default, maps raw ADC counts between `SensorValueAt0PSI` and `SensorValueAt50PSI` onto 0–50 PSI.
- Method 2 treats those two settings as voltages after the divider, multiplies the ADC voltage by 2 to undo the divider, and clamps the result to 0–50 PSI.

The shipped numbers, 412 counts at 0 PSI and 3182 counts at 50 PSI, belong to an older sensor. They do not match this 0.5–4.5 V transducer, and the full-scale PSI is whatever is stamped on the part. Set both calibration points from a known pressure after the sensor is installed. Zero pressure is about 0.5 V on the green wire, not 0 V.

## Parts

| Qty | Part | Link |
|---|---|---|
| 1 | ESP32 Relay X2 | [AliExpress](https://www.aliexpress.com/item/1005007334208641.html) |
| 2 | 12 V normally closed solenoid, 6.5 W | [AliExpress](https://www.aliexpress.com/item/1005006755761577.html) |
| 1 | 0.5–4.5 V pressure sensor, 1/8 NPT | [AliExpress](https://www.aliexpress.com/item/4001179110023.html) |
| 1 | 8 mm push-fit manifold, 6 ways or more | [AliExpress](https://www.aliexpress.com/item/1005001462976015.html) |
| 1 | 12 V supply, 2 A or more | |
| 1 | 2 A inline fuse | |
| 2 | 1N4007 | |
| 2 | 10 kΩ resistor | |
| 1 | 100 nF ceramic capacitor | |
| 1 | 1/8 NPT female to 8 mm push-fit, for the sensor | |
| 3 | Valve NPT to 8 mm push-fit | Match 1/8 or 1/4 NPT to the valves received |
| | 8 mm PU tube | Tank, manifold, and four tires |

The deflate outlet can vent open to air. The tank and the tires are already on the vehicle.

## BLE

BLE security is off by default. When it is turned on, pairing uses the passkey stored in `BTPinCode` (default `1234`). The message format is in [BLE_PROTOCOL.md](BLE_PROTOCOL.md).
