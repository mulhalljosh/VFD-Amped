# Amped Fabrication — Dual VFD PoE Controller

Shop-floor web controller for **two independent VFD channels** (pump = ch1, cooler = ch2). Amped Fabrication, Wenatchee, WA.

Locked v0.1: Waveshare **ESP32-S3-ETH + PoE Module (B)** (802.3af class 0/3), **one dual GP8413** @ `0x58` (VOUT0=pump, VOUT1=cooler), **companion current DAC** @ `0x59`/`0x5A`, dry active-high RUN into FWD–COM, VFD dry fault (closed=faulted, MCU active-low), temps **ambient / water in / water out**, isolated **RS-485 on the first PCB**, DIN rail box, hostname **amped-vfd.local**.

This repo is the firmware + docs scaffold. **Mock mode builds and serves the GUI with no board attached.**

## Architecture

```
                 ┌─────────────────────────────────────┐
   Browser /     │  web/   static UI + REST            │
   SCADA  ──────►│  GET /api/status  GET /api/temps    │
                 │  POST /api/vfd/1  POST /api/vfd/2   │
                 └──────────────────┬──────────────────┘
                                    │ X-Api-Key stub
                 ┌──────────────────▼──────────────────┐
                 │  app/  controller, failsafe, config │
                 │  heartbeat · interlock · NVS stub   │
                 └──────────────────┬──────────────────┘
                                    │
          ┌─────────────┬───────────┼───────────┬─────────────┐
          ▼             ▼           ▼           ▼             ▼
      hal/dac       hal/io      hal/temps   hal/rs485     net/
      GP8413        RUN/FAULT   ambient     RS-485 HW     MQTT stub
      + I-DAC       FWD–COM     water I/O   Modbus opt-in OTA later
```

| Layer | Owns | Must not own |
| --- | --- | --- |
| `hal/` | Pins, GP8413 + companion current-DAC I2C, GPIO, 1-Wire, UART | HTTP, setpoints, failsafe policy |
| `app/` | Modes, speed, interlock, heartbeat actions | I2C bytes, HTML |
| `web/` | Routes, JSON, static files | DAC codes |
| `net/` | MQTT publish, OTA, NVS blobs | VFD policy |

Mock vs hardware is a compile flag (`AMPED_MOCK`). Same API and UI.

## Why PlatformIO (Arduino), not ESP-IDF

The brain is a **W5500 SPI Ethernet** module, not ESP32 native EMAC. Arduino-ESP32 already has `WebServer`, LittleFS, `Wire`, USB-CDC, OneWire, and known W5500 bring-up on this Waveshare board. That is the shortest path to a flashable image.

HAL interfaces stay thin (GP8413 voltage + companion current DAC are real I2C writes, not PWM or a V/I transmitter) so an ESP-IDF port can replace `src/web/server.cpp` and the Arduino HAL bodies later without rewriting the controller or REST contract.

**Default development path is the native mock** (`make`), which does not need PlatformIO or an ESP32 toolchain.

## Features (v0.1 scaffold)

- Dual VFD panels: manual / **auto (temp-band)**, speed %, run, fault
- Live temps: ambient, water in, water out (~3–5 m probes)
- REST + single API-key (`X-Api-Key`); empty key = open LAN
- STOP and default failsafe → **0 V / 4 mA**, RUN open; heartbeat **15 s**
- Cooler-only-if-pump: software, default off
- MQTT publish stub only; OTA off (USB-C first)
- No PWM-fake analog; no secrets in git

## Repo layout

```
docs/hardware.md     electrical lock
docs/io-map.md       GPIO + analog scaling
docs/bom.md          v0.1 parts
OPEN_QUESTIONS.md    full v0.1 lock (none blocking merge)
src/hal/             GP8413 + current DAC, dry RUN relays, temps, RS-485
src/app/             controller + failsafe
src/web/             REST + HTTP
src/net/             MQTT / OTA / NVS stubs
data/www/            Web UI (LittleFS on target)
platformio.ini       native + esp32s3_poe
Makefile             hardware-free mock (g++)
secrets.example.h    copy to secrets.h locally
```

## Mock mode (no hardware)

Needs `g++` (C++17). From the repo root:

```bash
make            # build/amped-mock
make test       # API + failsafe self-tests
make run        # http://127.0.0.1:8080
```

```bash
curl -s http://127.0.0.1:8080/api/status
curl -s http://127.0.0.1:8080/api/temps
curl -s -X POST http://127.0.0.1:8080/api/vfd/1 \
  -H 'Content-Type: application/json' \
  -d '{"mode":"manual","speed_pct":42,"run":true}'
```

If `pio` is installed: `pio run -e native` (same `-DAMPED_MOCK=1` sources).

## Flash the ESP32-S3 PoE board

1. Install [PlatformIO](https://platformio.org/) (CLI or VS Code).
2. USB-C to the Waveshare board. **Do not** also feed PoE while USB is connected.
3. Copy `secrets.example.h` → `secrets.h` if you want compile-time Wi-Fi / API key defaults. Leave strings empty to keep the LAN open and use NVS later.
4. Build, upload firmware, upload the web filesystem:

```bash
pio run -e esp32s3_poe
pio run -e esp32s3_poe --target upload
pio run -e esp32s3_poe --target uploadfs
pio device monitor -e esp32s3_poe
```

Board env is `esp32-s3-devkitc-1` with 16 MB flash, OPI PSRAM, USB-CDC on boot — the stock PlatformIO name closest to the Waveshare ESP32-S3R8 ETH module.

Ethernet (W5500) bring-up is stubbed to DHCP in `src/web/server.cpp`. If ETH is down, optional Wi-Fi STA from `secrets.h` / NVS is the fallback. Serial prints the IP.

Open `http://<ip>/` or **http://amped-vfd.local/** once mDNS is up. First-boot analog is 0 V / 4 mA, RUN off. Flash over **USB-C** (do not also apply PoE). OTA stays disabled until later.

### Config (no secrets in git)

| Place | What |
| --- | --- |
| `secrets.h` (gitignored) | Optional compile-time Wi-Fi, API key, MQTT |
| NVS stub | Runtime settings (failsafe, interlock, heartbeat, key) |
| Environment | Never commit `.env` or `data/config.json` with real keys |

API auth: send `X-Api-Key: <key>`. If the configured key is empty, the stub accepts every request (lab default).

## REST

| Method | Path | Body / notes |
| --- | --- | --- |
| `GET` | `/api/status` | Channels, temps, heartbeat, mock flag |
| `GET` | `/api/temps` | Three probes |
| `POST` | `/api/vfd/1` | `{"mode":"manual"|"auto","speed_pct":0-100,"run":true}` — auto = temp-band |
| `POST` | `/api/vfd/2` | Same |
| `GET`/`POST` | `/api/settings` | Failsafe, interlock, heartbeat, analog path, outdoor/water bands |
| `POST` | `/api/heartbeat` | Refresh watchdog without changing setpoints |

Any authenticated `/api/*` call also refreshes the heartbeat.

## Failsafes

- **Zero (default, both channels)** — 0 V / 4 mA, RUN open
- **Hold / preset** — still in settings, not the factory default
- **STOP** — same analog drop (0 V / 4 mA), even if a speed setpoint is stored
- **Heartbeat** — any GUI / API / later HA call; **15 s**
- **Interlock** — software cooler-only-if-pump, default off
- **Auto** — temp-band stub (outdoor → pump, water out → cooler)

## License

MIT — see `LICENSE`.
