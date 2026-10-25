# Parts to buy

The relays are already on the ESP32 board. Buy the valves, the sensor, the manifold, and the fittings that join them. The tank and the tires are already on the vehicle.

## Main parts

| Qty | Buy | Why | Link |
|---|---|---|---|
| 1 | ESP32 Relay X2 | Controller. Two relays are onboard: outermost relay is GPIO16, the other is GPIO17. 12 V goes to the terminal marked VCC. The pin marked 5 V is an output. | [AliExpress](https://www.aliexpress.com/item/1005007334208641.html) |
| 2 | 12 V normally closed solenoid, 6.5 W | One inflate valve, one deflate valve. Coil wires have no polarity. Before buying adapters, check whether the air thread is 1/8 NPT or 1/4 NPT. | [AliExpress](https://www.aliexpress.com/item/1005006755761577.html) |
| 1 | Pressure sensor, 0.5–4.5 V, 1/8 NPT | Reads manifold pressure. Choose the PSI range you need. The number stamped on the hex is full scale: 0.5 V is zero, 4.5 V is that full scale. | [AliExpress](https://www.aliexpress.com/item/4001179110023.html) |
| 1 | 8 mm push-fit manifold, 6 ways or more | One shared air volume for the inflate valve, the deflate valve, the sensor, and four tires. An 8-way block leaves spare ports. | [AliExpress](https://www.aliexpress.com/item/1005001462976015.html) |

## Power and the sensor divider

| Qty | Buy | Where it goes |
|---|---|---|
| 1 | 12 V DC supply, 2 A or more | Board VCC and both relay commons. One valve draws about 0.54 A, and only one runs at a time. |
| 1 | Inline fuse, 2 A | On the 12 V wire, before the board. |
| 2 | 1N4007 diode | One across each valve coil. The band points toward the relay NO terminal. [Search](https://www.aliexpress.com/w/wholesale-1N4007.html) |
| 2 | 10 kΩ resistor | R5 in series with the sensor green wire, R6 from that junction to ground. [Search](https://www.aliexpress.com/w/wholesale-10k-resistor.html) |
| 1 | 100 nF ceramic capacitor | From the same junction to ground, at the board. [Search](https://www.aliexpress.com/w/wholesale-100nF-capacitor.html) |

## Air fittings

The manifold is 8 mm push-fit. The valves are NPT, and the sensor is 1/8 NPT male.

| Qty | Buy | Where it goes |
|---|---|---|
| 1 | 1/8 NPT female to 8 mm push-fit | Sensor into the manifold. [Search](https://www.aliexpress.com/w/wholesale-1-8-NPT-8mm-push-fit.html) |
| 3 | Valve thread to 8 mm push-fit | Tank into the inflate valve, inflate valve into the manifold, manifold into the deflate valve. Match 1/8 or 1/4 NPT to the valves you receive. |
| 1 | 8 mm PU air tube, a few meters | Tank, manifold, and four tires. [Search](https://www.aliexpress.com/w/wholesale-8mm-pu-tube.html) |

The deflate valve outlet can stay open to air. It does not need a fitting.

## Do not buy

- A separate relay module. This board already has both relays.
- A second pressure sensor. One sensor on the manifold reads all four tires.
- The air tank or the tires.
