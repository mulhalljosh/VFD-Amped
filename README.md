# Amped Fabrication — Dual VFD PoE Controller

Shop-floor web controller for **two independent VFD channels** (pump = ch1, cooler = ch2). Amped Fabrication, Wenatchee, WA.

Locked v0.1 hardware: Waveshare **ESP32-S3-ETH class** brain, **802.3af PoE** for logic, **GP8413** 15-bit I2C DAC for real 0–10 V (optional 4–20 mA path), run relays, fault optos, 3× DS18B20, optional isolated RS-485.

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
      GP8413        RUN/FAULT   DS18B20     Modbus        MQTT/OTA
      I2C DAC       relays      1-Wire      RTU master    stubs
```

| Layer | Owns | Must not own |
| --- | --- | --- |
| `hal/` | Pins, GP8413 registers, GPIO, 1-Wire, UART | HTTP, setpoints, failsafe policy |
| `app/` | Modes, speed, interlock, heartbeat actions | I2C bytes, HTML |
| `web/` | Routes, JSON, static files | DAC codes |
| `net/` | MQTT publish, OTA, NVS blobs | VFD policy |

Mock vs hardware is a compile flag (`AMPED_MOCK`). Same API and UI.

## Why PlatformIO (Arduino), not ESP-IDF

The brain is a **W5500 SPI Ethernet** module, not ESP32 native EMAC. Arduino-ESP32 already has `WebServer`, LittleFS, `Wire`, USB-CDC, OneWire, and known W5500 bring-up on this Waveshare board. That is the shortest path to a flashable image.

HAL interfaces stay thin (GP8413 is a real I2C DAC driver, not a PWM hack) so an ESP-IDF port can replace `src/web/server.cpp` and the Arduino HAL bodies later without rewriting the controller or REST contract.

**Default development path is the native mock** (`make`), which does not need PlatformIO or an ESP32 toolchain.

## Features (v0.1 scaffold)

- Dual VFD panels: manual/auto, speed %, run, fault
- Live temps: onboard + two waterproof DS18B20
- REST API + API-key stub (`X-Api-Key`)
- Failsafes: per-channel hold / 0% / preset; network heartbeat; optional cooler-only-if-pump
- MQTT publish stub, OTA stub, NVS config stub
- No PWM-fake analog; no secrets in git

## Repo layout

```
docs/hardware.md     electrical lock
docs/io-map.md       GPIO + analog scaling
docs/bom.md          v0.1 parts
OPEN_QUESTIONS.md    decisions for Josh
src/hal/             GP8413, relays, temps, RS-485
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

Open `http://<ip>/`. First-boot analog is 0 V / 4 mA, RUN off.

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
| `POST` | `/api/vfd/1` | `{"mode":"manual"|"auto","speed_pct":0-100,"run":true}` |
| `POST` | `/api/vfd/2` | Same |
| `GET`/`POST` | `/api/settings` | Failsafe, interlock, heartbeat, analog path |
| `POST` | `/api/heartbeat` | Refresh watchdog without changing setpoints |

Any authenticated `/api/*` call also refreshes the heartbeat.

## Failsafes

- **Hold** — keep last commanded speed/run after timeout
- **Zero** — 0% analog, RUN off (default)
- **Preset** — configured `%` (RUN off unless you set `preset_run`)
- **Interlock** — if enabled, cooler RUN is forced off unless pump is running and not faulted

## License

MIT — see `LICENSE`.
