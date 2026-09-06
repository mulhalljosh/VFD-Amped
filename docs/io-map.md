# I/O map (locked v0.1)

Waveshare **ESP32-S3-ETH + PoE Module (B)**. Pin numbers are ESP32-S3 GPIO, not Pico header silkscreen. Hostname **amped-vfd.local**.

Firmware constants live in `src/hal/board.h`. Treat that header as the single source of truth if a later PCB spins a pin.

## Reserved by the brain (do not reuse)

| GPIO | Function |
| --- | --- |
| 9, 10, 11, 12, 13, 14 | W5500 Ethernet SPI (RST, INT, MOSI, MISO, SCLK, CS) |
| 19, 20 | USB-JTAG / USB-CDC |
| 26–37 | Octal flash + OPI PSRAM (ESP32-S3R8) |
| 0 | BOOT strap / button |
| 43, 44 | UART0 (USB serial on many images) |
| 45, 46 | Strapping — leave alone |

TF card (4/5/6/7) and camera DVP are unused. Camera pins 1/2/15/18/38–42/47/48 are available **because v0.1 does not mount a camera**.

## Application map

| Signal | GPIO | Dir | Electrical | Notes |
| --- | --- | --- | --- | --- |
| I2C SDA | 16 | OD | 3.3 V, 4.7 kΩ pull-up | GP8413 @ 0x58 + current DAC @ 0x59/0x5A |
| I2C SCL | 17 | OD | 3.3 V, 4.7 kΩ pull-up | 400 kHz target |
| 1-Wire DQ | 8 | OD | 3.3 V, 4.7 kΩ pull-up | 3× DS18B20 |
| RUN1 pump | 38 | out | 3.3 V → MOSFET → 5 V coil | Active-high; dry FWD–COM |
| FAULT1 pump | 39 | in | Opto, MCU pull-up | VFD dry, faulted=closed → MCU low |
| RUN2 cooler | 40 | out | same as RUN1 | Active-high; dry FWD–COM |
| FAULT2 cooler | 41 | in | same as FAULT1 | Map ALM/FA per drive; no reset out |
| RS485 TX | 1 | out | to isolator TXD | UART1 — **populated** |
| RS485 RX | 2 | in | from isolator RXD | UART1 |
| RS485 DE/RE | 42 | out | high = transmit | Modbus software opt-in |
| Status LED | 21 | out | optional | Not required for v0.1 |

## I2C devices

| Address | A2 A1 A0 | Device | Channel map |
| --- | --- | --- | --- |
| `0x58` | 0 0 0 | GP8413 (locked) | VOUT0 = pump 0–10 V, VOUT1 = cooler 0–10 V |
| `0x59` | 1 0 0 | Companion current DAC (locked) | 1-ch: pump 4–20 mA. Dual-ch part: IOUT0 = pump, IOUT1 = cooler |
| `0x5A` | 0 1 0 | Second 1-ch current DAC | Cooler 4–20 mA when using two GP8313 / GP8600 |

Linearin family straps A0/A1/A2 select `0x58`–`0x5F`. Do not put a current DAC on `0x58` — that address is the voltage GP8413.

Firmware writes 4–20 mA as I2C codes to `0x59` / `0x5A` (and dual-channel registers on `0x59`). It does **not** treat GP8413 VOUT as a V/I transmitter input.

## RUN (locked)

| Item | Lock |
| --- | --- |
| VFD terminals | **FWD–COM** dry pair |
| MCU | GPIO high → MOSFET on → 5 V coil energized → NO contacts close |
| Stop / fail-safe | GPIO low or power loss → coil off → contacts open → FWD open from COM |
| Not used | Sourced 24 V DI from this controller |

Field terminals “Pump RUN A/B” / “Cooler RUN A/B” are the dry FWD–COM contacts.

## Analog scaling

| Command `speed_pct` | 15-bit code | GP8413 VOUT (`0x58`) | Companion DAC IOUT (`0x59`/`0x5A`) |
| --- | --- | --- | --- |
| 0 | 0 | 0.000 V | 4.00 mA |
| 50 | 16383 | 5.000 V | 12.00 mA |
| 100 | 32767 | 10.000 V | 20.00 mA |

**STOP** (`run = false`) and channel fault always write **0 V / 4 mA**. **4 mA = 0%**. Comms-loss failsafe default is the same (0% + RUN open). Hold/preset remain settings options only.

## 1-Wire index

| Index | JSON `id` | Label | Role |
| --- | --- | --- | --- |
| 0 | `ambient` | Ambient | Onboard outdoor / panel |
| 1 | `water_in` | Water in | Waterproof, ~3–5 m |
| 2 | `water_out` | Water out | Waterproof, ~3–5 m |

Assign ROM64 values in NVS after first scan (`config.temp_rom[i]`). Auto temp-band: outdoor (`ambient`) → pump, `water_out` → cooler.

## RS-485 (hardware on first PCB)

| Setting | v0.1 lock |
| --- | --- |
| Transceiver | Isolated, **populated** |
| Baud | 9600 |
| Format | 8E1 (confirm per drive) |
| Mode | Modbus RTU master, **software opt-in** |
| Drive family | **mix / configurable** |
| Slaves | none commissioned |

## Connector intent (carrier)

Field side, Phoenix-style 5.08 mm (suggested):

1. PE / shield
2. Pump 0–10 V+, 0–10 V−
3. Pump 4–20 mA+, 4–20 mA− (if fitted)
4. Pump RUN A/B (dry FWD–COM; closed = run)
5. Pump FAULT A/B (VFD dry, closed = fault, into opto)
6. Cooler — same set as pump
7. RS-485 A/B/GND/SH (populated)
8. DIN 24 V+ / 0 V (current-DAC compliance only)
9. 1-Wire: water in + water out pigtails (DQ, 3V3, GND), ~3–5 m

Logic side: Pico header + PoE RJ45 + USB-C. No field 24 V on the Pico pins.
