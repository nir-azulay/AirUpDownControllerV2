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

| Relay | Fixed GPIO | Active level | Load |
|---|---|---|---|
| Relay 1, outermost | 16 | High = on | Inflate valve Y1 |
| Relay 2 | 17 | High = on | Deflate valve Y2 |

Those two GPIOs are soldered to the relays. Do not move them. The firmware defaults are already 16 and 17, active high. The screw terminals you wire are COM, NO, and NC.

The other coil wire of each valve returns to supply ground, together with the board GND pin. Only one valve runs at a time. Firmware holds both relay pins low at boot and whenever air is stopped.

Put a 1N4007 across each coil. The band points toward the relay NO side, which is the positive side of the coil.

If this board was flashed when the defaults were still 25 and 26, the saved configuration overrides the new defaults. On the configuration page set the compressor pin to 16 and the free pin to 17, then save. That only tells the firmware to drive the pins the relays already use.

## Pressure sensor

The transducer output is 0.5 V at zero pressure and 4.5 V at the full-scale PSI stamped on the hex. Do not connect the green wire straight to an ESP32 pin.

| Sensor wire | Goes to |
|---|---|
| Red | Board 5 V output |
| Black | Board GND, which is the same ground as the 12 V supply negative |
| Green | R5, then the divider into the sensor GPIO |

R5 and R6 are 10 kΩ. R5 is in series with the green wire. R6 goes from the divider output to ground. C3 is 100 nF from that same output to ground. The divider presents about 0.25 V to 2.25 V to the pin.

The sensor is not a relay pin. Every GPIO is labeled on the back of the board. Connect the divided green wire to **GPIO34**. That pin is ADC1, so Wi-Fi does not block the reading. Leave the relays on 16 and 17.

A board that already saved its settings may still have sensor pin 27. On the configuration page set the sensor GPIO to 34, then save. GPIO27 is ADC2 and the reading can be wrong while Wi-Fi is connected.

## Calibration

Enter the PSI stamped on the sensor as **Sensor full scale** on the configuration page. Both valves stay closed until that value is greater than zero.

The phone apps still use the existing names `SensorValueAt0PSI` and `SensorValueAt50PSI`. Those names are unchanged. The second value is the sensor reading at full scale, and the PSI span is the full-scale field above.

- Method 1, the default, maps raw ADC counts between those two settings across the full-scale PSI.
- Method 2 treats those two settings as voltages after the divider and multiplies the ADC voltage by 2. For this sensor the voltages are 0.250 at zero and 2.250 at full scale.

A new device starts at 310.227 counts and 2792.045 counts. That is 0.25 V and 2.25 V on the divider, which is 0.5 V and 4.5 V on the green wire. A device that already saved 412 and 3182 keeps those saved numbers until they are changed on the configuration page. Zero pressure is about 0.5 V on the green wire, not 0 V.

## Valve limits

These limits are on the configuration page. They are not BLE fields, so the phone apps do not send or receive them.

| Setting | Default | What it does |
|---|---|---|
| Sensor full scale (PSI) | 0 | The stamp on the sensor. Valves stay closed while this is 0. |
| Max pressure (PSI) | 50 | A higher target is rejected. A reading above this closes both valves. |
| Max valve open (sec) | 120 | Closes both valves if a start command runs longer than this. |

A target must be greater than 0 and no higher than the lower of the maximum pressure and the sensor full scale. If five readings in a row disagree by more than 1 PSI, or the sensor signal is missing, both valves close. The phone still receives the same pressure and operation-mode message. Stop is operation mode 1.

## Setup access

If the home Wi-Fi does not connect, the board opens an access point named with the device name. The factory access-point password is `AirUpDown`. Change it on the configuration page. It must be at least 8 characters. The serial log does not print it.

The configuration page leaves the Wi-Fi password blank. Leave it blank to keep the saved password, or type a new one. The BLE settings message still includes `wifipassword`, because the phone apps expect that field.

## Parts

What to buy, with links, is in [PARTS.md](PARTS.md).

## BLE

BLE security is off by default. When it is turned on, pairing uses the passkey stored in `BTPinCode` (default `1234`). The message format is in [BLE_PROTOCOL.md](BLE_PROTOCOL.md). The messages use the same fields the phone apps already know. Maximum pressure, valve time, sensor full scale, and the access-point password are not part of those messages.
