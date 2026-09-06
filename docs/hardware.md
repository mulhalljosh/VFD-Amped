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
         ┌───────▼─┐   │    │    └──────► isolated RS-485
         │ GP8413  │   │    │              Modbus RTU master
         │ 15-bit  │   │    │              (optional)
         │ dual DAC│   │    │
         └──┬───┬──┘   │    │
        VOUT0 VOUT1    │    ├─ RUN1 / RUN2  relay drivers
         0-10V 0-10V   │    └─ FAULT1 / FAULT2 opto-in
            │    │     │
     pump AO│    │cooler AO
            │    │
            └────┼──────────────► optional 4–20 mA path
                 │                 (see Current loops)
                 │
                 └─ 3× DS18B20 on one 1-Wire bus
                    (1 onboard + 2 waterproof)

        DIN 24 V (optional, isolated) ──► current-loop transmitters only
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

Firmware HAL (`src/hal/dac.cpp`) speaks this protocol. Mock mode stores the same 15-bit codes and reports volts / mA without a bus.

### 4–20 mA

The GP8413 silicon is **voltage-only** (0–5 V / 0–10 V). The product lock still allows 0–10 V **and/or** 4–20 mA per channel. v0.1 software maps speed to both:

- Voltage: `0–10 V` linear with `speed_pct`
- Current: `4–20 mA` linear with `speed_pct` (`0% → 4 mA`, `100% → 20 mA`)

Hardware realization is an open (see `OPEN_QUESTIONS.md`):

1. **Preferred interpretation of “via GP8413”:** same 0–10 V into an isolated V/I transmitter, loop-powered from the optional DIN 24 V rail.
2. **Alternate:** second Linearin current DAC (GP8313 / GP8600) on I2C `0x59`. The HAL already has a hook for a companion current write.

Do not share PoE logic ground with the 24 V loop supply.

## Digital I/O

| Signal | Direction | Isolation | Notes |
| --- | --- | --- | --- |
| RUN1 (pump) | MCU → VFD | Relay dry contact | VFD FWD / DI “run” |
| RUN2 (cooler) | MCU → VFD | Relay dry contact | Independent of ch1 |
| FAULT1 | VFD → MCU | Optocoupler | Active-low at MCU (opto pulls down) |
| FAULT2 | VFD → MCU | Optocoupler | Same |

Relay coils are driven from the **5 V logic rail** through a MOSFET + flyback. Contacts are dry into the VFD — do not source 24 V from PoE.

On a channel fault the controller drops that RUN relay and applies the configured failsafe analog action.

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
| Logic | PoE 802.3af → 5 V → 3.3 V | ESP32, W5500, GP8413 I2C, 1-Wire, relay coils | Field 24 V, RS-485, opto, current loops |
| Field analog (optional) | DIN 24 V | 4–20 mA transmitters only | Logic |
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
- Driving VFD run from a GPIO without a relay
- Mixing DIN 24 V return with PoE ground
