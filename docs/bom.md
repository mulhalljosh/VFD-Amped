# Bill of materials — v0.1 controller

Quantities are **per assembled controller**, not a production reel. Part numbers are representative; Amped Fabrication purchasing may substitute equivalents that keep the locked interfaces.

## Brain and power

| Qty | Item | Example / notes | Est. |
| --- | --- | --- | --- |
| 1 | ESP32-S3 Ethernet + PoE brain | Waveshare **ESP32-S3-ETH** + **PoE Module (B)** (SKU class 28771 / wiki ESP32-S3-ETH). 802.3af PD, W5500, Pico header | Buy |
| 1 | PoE source | 802.3af injector or switch port, for logic only | Site |
| 1 | DIN 24 V PSU (optional) | 24 VDC, isolated, **companion current-DAC analog / loop compliance only**. Size for 2× 20 mA + DAC quiescent | Site |
| 1 | Logic fuse / PTC | On 5 V after PoE module | Proto |

## Analog (real DAC)

| Qty | Item | Example / notes | Est. |
| --- | --- | --- | --- |
| 1 | GP8413 dual 15-bit I2C 0–10 V DAC | DFRobot Gravity DFR1073 **or** Linearin GP8413. Addr **`0x58`**. VOUT0 = pump, VOUT1 = cooler. **Not** a PWM RC filter | Buy |
| 2 | Companion current DAC (4–20 mA) | Linearin **GP8313** or **GP8600** class, I2C. Strap **`0x59` = pump**, **`0x5A` = cooler**. One dual-channel current DAC at `0x59` may replace the pair. **Not** a V/I transmitter from GP8413 | Buy |
| 2 | 0.1 µF on each GP8413 VOUT | Datasheet requirement if using bare GP8413 | |
| 2 | I2C pull-up 4.7 kΩ | 3.3 V, shared bus | |

## Digital I/O

| Qty | Item | Example / notes | Est. |
| --- | --- | --- | --- |
| 2 | SPDT relay + MOSFET driver | 5 V coil, **dry FWD–COM**. Active-high GPIO energizes coil → NO closes → run. AO3400 + 1N4148 flyback or a 2-ch relay board | Buy |
| 2 | Fault optocoupler | PC817 / TLP281 class, VFD alarm into LED side with series R | Buy |
| 4 | TVS / clamp on field lines | On RUN contacts and fault inputs as appropriate | |

## Temperature

| Qty | Item | Example / notes | Est. |
| --- | --- | --- | --- |
| 1 | DS18B20 TO-92 | Onboard / panel | Buy |
| 2 | DS18B20 waterproof | 1 m+ pigtail, food/process grade as needed | Buy |
| 1 | 4.7 kΩ 1-Wire pull-up | To 3.3 V | |

## Optional comms

| Qty | Item | Example / notes | Est. |
| --- | --- | --- | --- |
| 1 | Isolated RS-485 transceiver | ISO3082, MAX14878, or Waveshare isolated RS-485 Pico hat | Optional |
| 1 | 120 Ω termination | Switchable | |

## Mechanics / interconnect

| Qty | Item | Example / notes | Est. |
| --- | --- | --- | --- |
| 1 | DIN enclosure | Enough for brain + relays + DAC + terminals | Buy |
| 1 | Carrier / proto / PCB | Pico header female, field terminals 5.08 mm | TBD |
| 1 | Terminal set | Analog, run, fault, RS-485, 24 V, 1-Wire | Buy |
| — | Hook-up / shielded analog pair | Drain to PE at one end | |

## Not used (do not buy for v0.1)

- OV2640 / OV5640 camera
- TF card (unless you want local logs later)
- Extra GP8403/PWM “0–10 V” boards
- 0–10 V → 4–20 mA V/I transmitters (4–20 mA is the companion current DAC)
- Non-isolated TTL-to-RS485 dongles on the VFD cable

## Consumed by the VFD (customer)

Each channel still needs a VFD that accepts **0–10 V and/or 4–20 mA** analog speed plus a digital RUN and a fault/alarm output. Drive family is not locked — see `OPEN_QUESTIONS.md`.
