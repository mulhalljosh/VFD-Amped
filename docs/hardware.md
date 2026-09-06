# Hardware (locked v0.1)

Amped Fabrication dual-VFD PoE controller. Brain is a **Waveshare ESP32-S3-ETH + PoE Module (B)**. Field I/O lives on a carrier in a **DIN rail box**. This document is the electrical lock, not a finished gerber.

## Block diagram

```
                    802.3af PoE
                         │
                    ┌────▼────┐
                    │ PoE PD  │  logic 5 V only
                    │ module  │
                    └────┬────┘
                         │ 5 V
              ┌──────────▼──────────┐
              │ Waveshare ESP32-S3  │
              │ ETH (W5500 SPI)     │
              │ USB-C debug / flash │
              └──┬────┬────┬────┬───┘
           I2C   │  1W │GPIO│UART│
                 │     │    │    │
         ┌───────▼────────┐  │    │    └──────► isolated RS-485
         │ I2C analog     │  │    │              Modbus RTU master
         │                │  │    │              (populated; map opt-in)
         │ GP8413 @ 0x58  │  │    ├─ RUN1 / RUN2  MOSFET → relay
         │  VOUT0 pump    │  │    │  dry FWD–COM
         │  VOUT1 cooler  │  │    └─ FAULT1 / FAULT2 opto (faulted=closed)
         │                │  │
         │ Current DAC    │  │
         │  GP8313/GP8600 │  │
         │  @ 0x59 / 0x5A │  │
         │  4–20 mA/ch    │  │
         └────┬──────┬────┘  │
              │      │       │
         0–10 V   4–20 mA    └─ 3× DS18B20 (ambient / water in / water out)
         + loops                waterproof leads ~3–5 m

        DIN 24 V (optional, isolated) ──► current-DAC analog/compliance only
```

## Brain — Waveshare ESP32-S3-ETH + PoE

| Item | Lock |
| --- | --- |
| MCU | ESP32-S3R8, Xtensa LX7 dual-core @ 240 MHz |
| Memory | 8 MB OPI PSRAM, 16 MB flash |
| Ethernet | W5500, 10/100, SPI (not IDF native EMAC) |
| PoE | Waveshare **PoE Module (B)**, IEEE **802.3af class 0/3**, logic rail only |
| Enclosure | **DIN rail box** (first build) |
| Hostname | **amped-vfd.local** |
| Debug | USB Type-C, USB-CDC / USB-JTAG (GPIO19 / GPIO20) |
| Expansion | Pico-compatible header; camera and TF slot unused in v0.1 |

On-board W5500 SPI (do not reuse):

| W5500 | GPIO |
| --- | --- |
| MOSI | 11 |
| MISO | 12 |
| SCLK | 13 |
| CS | 14 |
| RST | 9 |
| INT | 10 |

TF-card SPI (GPIO4/5/6/7) and the DVP camera bus are left free. Do not fight the octal flash/PSRAM pins **GPIO26–GPIO37**.

**Do not** power USB-C and PoE at the same time — Waveshare documents this as a damage risk.

## Analog outputs — GP8413 (real DAC, not PWM)

v0.1 analog voltage is **one dual** Linearin / DFRobot **GP8413** (both VFDs). PWM-filtered “fake 0–10 V” is out of spec. **STOP** writes **0 V / 4 mA** (not hold). **4 mA = 0%**.

| Property | Value |
| --- | --- |
| Interface | I2C, default address `0x58` (A2/A1/A0 = 000) |
| Channels | VOUT0 = pump (VFD ch1), VOUT1 = cooler (VFD ch2) |
| Codes | 0…`0x7FFF` (32767) |
| Range | firmware sets 0–10 V (`0x77` to range register `0x01`) |
| Transfer | `VOUT = 10 V × code / 0x7FFF` |
| Registers | ch0 = `0x02`, ch1 = `0x04`, LSB then MSB |

Firmware HAL (`src/hal/dac.cpp`) speaks this protocol. Mock mode stores the same 15-bit codes and reports volts without a bus.

### 4–20 mA — companion current DAC (locked)

GP8413 does **not** generate loop current. 4–20 mA is a **separate I2C current DAC** (Linearin **GP8313 / GP8600 class**), not a V/I transmitter hung on GP8413 VOUT.

| Property | Lock |
| --- | --- |
| Family | GP8313 (15-bit, 4–20 mA) or GP8600 (16-bit, 0–10 V / 4–20 mA used as current) |
| Bus | Same I2C as GP8413 (GPIO16/17) |
| Scaling | `4 mA + 16 mA × speed_pct / 100` (`0% → 4 mA`, `100% → 20 mA`) |
| Codes | 15-bit `0…0x7FFF` in firmware (GP8600 16-bit mapping is a bring-up detail if that exact chip is purchased) |

**Address straps** (Linearin A2/A1/A0, same `0x58`–`0x5F` family as GP8413):

| A2 | A1 | A0 | Addr | Device |
| --- | --- | --- | --- | --- |
| 0 | 0 | 0 | `0x58` | GP8413 voltage (locked) |
| 1 | 0 | 0 | `0x59` | Current DAC — pump 4–20 mA (1-ch) **or** dual current IOUT0/IOUT1 |
| 0 | 1 | 0 | `0x5A` | Current DAC — cooler 4–20 mA (second 1-ch GP8313/GP8600) |

**Channel map**

- Two 1-ch parts (typical GP8313 / GP8600): `0x59` = pump IOUT, `0x5A` = cooler IOUT.
- One dual-channel current DAC at `0x59`: IOUT0 / ch0 = pump, IOUT1 / ch1 = cooler. Leave `0x5A` unpopulated.

The HAL writes **current codes to the companion DAC address(es)** as real I2C transactions. It does not derive 4–20 mA from GP8413 voltage. DIN 24 V (isolated) feeds the current DAC’s analog / loop-compliance rail only. Do not share that return with PoE logic ground.

## Digital I/O

| Signal | Direction | Isolation | Notes |
| --- | --- | --- | --- |
| RUN1 (pump) | MCU → VFD | Relay **dry** FWD–COM | Active-high GPIO; coil on = run |
| RUN2 (cooler) | MCU → VFD | Relay **dry** FWD–COM | Independent of ch1 |
| FAULT1 | VFD → MCU | Optocoupler | VFD **dry** contact; **faulted = closed**; MCU **active-low** |
| FAULT2 | VFD → MCU | Optocoupler | Same. Map ALM / FA / MA–MB per drive at commission |

### RUN polarity (locked)

RUN is a **dry contact into the VFD FWD–COM pair**. The MCU does not source 24 V onto the drive digital input.

1. Firmware drives RUN GPIO **active-high**.
2. MOSFET turns on → **5 V logic-rail relay coil energizes**.
3. Normally-open contacts **close**, tying VFD **FWD** to **COM** → run.
4. GPIO low, power loss, or a dropped coil **opens** the contacts → stop (fail-safe).

Boot leaves both RUN GPIOs low so the relays are de-energized. Do not wire a sourced 24 V DI from this board. On STOP or a channel fault the controller de-energizes that RUN relay and writes **0 V / 4 mA**. **No fault-reset output** in v0.1.

### Fault sense (locked)

The VFD presents a **dry contact** that is **closed when faulted**. That closure drives the opto LED; the MCU pin is pulled **low** (active-low). Terminal name (ALM, FA, MA–MB, …) is chosen **per drive at commission** — family is mix / configurable.

## Temperatures

One 1-Wire bus, 4.7 kΩ pull-up to 3.3 V:

| JSON `id` | Type | Role |
| --- | --- | --- |
| `ambient` | DS18B20 TO-92 onboard | Outdoor / panel ambient |
| `water_in` | Waterproof DS18B20, **~3–5 m** | Water in |
| `water_out` | Waterproof DS18B20, **~3–5 m** | Water out |

ROM IDs are stored in NVS once assigned. Auto mode uses a **temp-band stub** (outdoor → pump, water out → cooler).

## Isolated RS-485 (populated)

UART1 + DE/RE through an isolated transceiver (ISO3082 / MAX14878 class) **on the first PCB**. Firmware brings the UART up; **Modbus RTU master is opt-in** until a drive map is commissioned (mix / configurable).

## Power domains

| Domain | Source | Feeds | Isolated from |
| --- | --- | --- | --- |
| Logic | PoE 802.3af → 5 V → 3.3 V | ESP32, W5500, GP8413 + current-DAC I2C, 1-Wire, relay coils | Field 24 V, RS-485, opto, current loops |
| Field analog (optional) | DIN 24 V | Companion current-DAC analog / loop compliance only | Logic |
| VFD | Customer 3-phase / VFD supply | Motors | Everything on this board |

PoE **802.3af class 0/3**. **Measure current with both RUN coils energized** before calling the budget done. Keep analog loop power off PoE.

## Failsafes and control (locked)

- Default failsafe **both channels: 0% analog + RUN open**
- Heartbeat: any GUI / API / later HA client; **15 s** timeout
- Cooler-only-if-pump: **software only**, default off
- Auto = **temp-band** (outdoor / water); law is stubbed
- STOP / fault → **0 V / 4 mA**, RUN open
- Auth: single API key. MQTT: publish stub only. OTA: off (USB-C first)
- mDNS: **amped-vfd.local**

## What is not in v0.1

- Camera, TF card, RGB show LED as a product feature
- PWM analog
- Leaving RS-485 unpopulated (transceiver is on the first PCB)
- Fault-reset output
- Hardwired cooler-only-if-pump series contact
- Driving VFD run from a GPIO without a relay, or sourcing 24 V onto FWD
- 4–20 mA via V/I transmitter from GP8413 0–10 V (rejected — companion current DAC only)
- Mixing DIN 24 V return with PoE ground
