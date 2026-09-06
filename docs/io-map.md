# I/O map (locked v0.1)

Waveshare **ESP32-S3-ETH / ESP32-S3-POE-ETH** class. Pin numbers are ESP32-S3 GPIO, not Pico header silkscreen.

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
| I2C SDA | 16 | OD | 3.3 V, 4.7 kΩ pull-up | GP8413 (+ optional current DAC) |
| I2C SCL | 17 | OD | 3.3 V, 4.7 kΩ pull-up | 400 kHz target |
| 1-Wire DQ | 8 | OD | 3.3 V, 4.7 kΩ pull-up | 3× DS18B20 |
| RUN1 pump | 38 | out | 3.3 V → MOSFET → 5 V relay | Active high |
| FAULT1 pump | 39 | in | Opto, MCU pull-up | Active low = fault |
| RUN2 cooler | 40 | out | same as RUN1 | Active high |
| FAULT2 cooler | 41 | in | same as FAULT1 | Active low = fault |
| RS485 TX | 1 | out | to isolator TXD | UART1 |
| RS485 RX | 2 | in | from isolator RXD | UART1 |
| RS485 DE/RE | 42 | out | high = transmit | |
| Status LED | 21 | out | optional | Not required for v0.1 |

## I2C devices

| Address | Device | Channel map |
| --- | --- | --- |
| `0x58` | GP8413 #1 (locked) | VOUT0 = VFD1 pump 0–10 V, VOUT1 = VFD2 cooler 0–10 V |
| `0x59` | Optional companion DAC | Current-loop codes if a GP8313/second GP8413+V/I is fitted |

GP8413 hardware address straps A0/A1/A2 select `0x58`–`0x5F`.

## Analog scaling

| Command `speed_pct` | GP8413 code | VOUT | Loop current (if enabled) |
| --- | --- | --- | --- |
| 0 | 0 | 0.000 V | 4.00 mA |
| 50 | 16383 | 5.000 V | 12.00 mA |
| 100 | 32767 | 10.000 V | 20.00 mA |

`run = false` still writes analog 0% unless the channel failsafe is **hold** during a comms-loss event.

## 1-Wire index

| Index | JSON `id` | Default label |
| --- | --- | --- |
| 0 | `onboard` | Onboard |
| 1 | `probe1` | Probe 1 |
| 2 | `probe2` | Probe 2 |

Assign ROM64 values in NVS after first scan (`config.temp_rom[i]`).

## RS-485 defaults (stub)

| Setting | v0.1 default |
| --- | --- |
| Baud | 9600 |
| Format | 8E1 (common VFD; confirm per drive) |
| Mode | Modbus RTU master |
| Slaves | none commissioned |

## Connector intent (carrier)

Field side, Phoenix-style 5.08 mm (suggested):

1. PE / shield
2. Pump 0–10 V+, 0–10 V−
3. Pump 4–20 mA+, 4–20 mA− (if fitted)
4. Pump RUN A/B (dry)
5. Pump FAULT+/FAULT− (from VFD, into opto)
6. Cooler — same set as pump
7. RS-485 A/B/GND/SH
8. DIN 24 V+ / 0 V (loops only)
9. 1-Wire waterproof pigtails (DQ, 3V3, GND) ×2

Logic side: Pico header + PoE RJ45 + USB-C. No field 24 V on the Pico pins.
