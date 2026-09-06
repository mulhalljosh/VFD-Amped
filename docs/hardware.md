# Hardware (locked v0.1)

Amped Fabrication dual-VFD PoE controller. Brain is a **Waveshare ESP32-S3-ETH class** board with the external **802.3af PoE module**. Field I/O lives on a carrier / DIN interconnect — this document is the electrical lock, not a finished PCB gerber.

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
         │                │  │    │              (optional)
         │ GP8413 @ 0x58  │  │    ├─ RUN1 / RUN2  MOSFET → relay
         │  VOUT0 pump    │  │    │  dry FWD–COM
         │  VOUT1 cooler  │  │    └─ FAULT1 / FAULT2 opto-in
         │                │  │
         │ Current DAC    │  │
         │  GP8313/GP8600 │  │
         │  @ 0x59 / 0x5A │  │
         │  4–20 mA/ch    │  │
         └────┬──────┬────┘  │
              │      │       │
         0–10 V   4–20 mA    └─ 3× DS18B20 on one 1-Wire bus
         + loops              (1 onboard + 2 waterproof)

        DIN 24 V (optional, isolated) ──► current-DAC analog/compliance only
```

## Brain — Waveshare ESP32-S3-ETH + PoE

| Item | Lock |
| --- | --- |
| MCU | ESP32-S3R8, Xtensa LX7 dual-core @ 240 MHz |
| Memory | 8 MB OPI PSRAM, 16 MB flash |
| Ethernet | W5500, 10/100, SPI (not IDF native EMAC) |
| PoE | Waveshare PoE Module (B), **IEEE 802.3af**, logic rail only |
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

v0.1 analog speed is a **Linearin / DFRobot GP8413** 15-bit I2C DAC. PWM-filtered “fake 0–10 V” is out of spec.

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
| FAULT1 | VFD → MCU | Optocoupler | Active-low at MCU (opto pulls down) |
| FAULT2 | VFD → MCU | Optocoupler | Same |

### RUN polarity (locked)

RUN is a **dry contact into the VFD FWD–COM pair**. The MCU does not source 24 V onto the drive digital input.

1. Firmware drives RUN GPIO **active-high**.
2. MOSFET turns on → **5 V logic-rail relay coil energizes**.
3. Normally-open contacts **close**, tying VFD **FWD** to **COM** → run.
4. GPIO low, power loss, or a dropped coil **opens** the contacts → stop (fail-safe).

Boot leaves both RUN GPIOs low so the relays are de-energized. Do not wire a sourced 24 V DI from this board. On a channel fault the controller de-energizes that RUN relay and applies the configured failsafe analog action.

## Temperatures

One 1-Wire bus, 4.7 kΩ pull-up to 3.3 V:

| Probe | Type | Role (default labels) |
| --- | --- | --- |
| `onboard` | DS18B20 TO-92 | Panel / PCB |
| `probe1` | Waterproof DS18B20 | Process |
| `probe2` | Waterproof DS18B20 | Ambient / secondary |

ROM IDs are stored in NVS once assigned. Mock mode synthesizes slowly changing values.

## Isolated RS-485 (optional)

UART1 + DE/RE through an isolated transceiver (ISO3082 / MAX14878 class). Firmware is a **Modbus RTU master stub** only — no vendor register map in v0.1.

## Power domains

| Domain | Source | Feeds | Isolated from |
| --- | --- | --- | --- |
| Logic | PoE 802.3af → 5 V → 3.3 V | ESP32, W5500, GP8413 + current-DAC I2C, 1-Wire, relay coils | Field 24 V, RS-485, opto, current loops |
| Field analog (optional) | DIN 24 V | Companion current-DAC analog / loop compliance only | Logic |
| VFD | Customer 3-phase / VFD supply | Motors | Everything on this board |

PoE class 0/3 budget is tight once two relay coils and the W5500 are on. Keep analog loop power off PoE.

## Failsafes (hardware + firmware)

- Per-channel analog action on comms loss: **hold**, **0%**, or **preset**
- Network heartbeat (any authenticated `/api/*` refreshes it; timeout applies failsafe)
- Optional **cooler-only-if-pump** interlock (software; hardwire is an open)
- Fault opto forces RUN off for that channel
- DAC comes up at 0 V / 4 mA until the app writes a command

## What is not in v0.1

- Camera, TF card, RGB show LED as a product feature
- PWM analog
- Non-isolated RS-485
- Driving VFD run from a GPIO without a relay, or sourcing 24 V onto FWD
- 4–20 mA via V/I transmitter from GP8413 0–10 V (rejected — companion current DAC only)
- Mixing DIN 24 V return with PoE ground
