# Locked decisions (v0.1)

Amped Fabrication, Wenatchee WA. Josh signed off the full set. **Do not re-open.** Electrical and software locks live in `docs/` and firmware defaults.

**None blocking merge.**

| # | Lock |
| --- | --- |
| 1 | **4–20 mA = companion current DAC** (GP8313 / GP8600 class). GP8413 @ `0x58` is 0–10 V only. `0x59` = pump (or dual IOUT0/IOUT1), `0x5A` = cooler for two 1-ch parts. Not a V/I transmitter from GP8413. |
| 2 | **One dual GP8413** for both VFDs: VOUT0 = pump, VOUT1 = cooler. |
| 3 | **STOP → analog 0 V / 4 mA** (RUN open). Not hold-last on stop. |
| 4 | **4 mA = 0%** (standard). 20 mA = 100%. |
| 5 | **RUN = dry FWD–COM, active-high.** GPIO high → coil on → NO closes FWD to COM. Power loss / coil off = stop. |
| 6 | **Fault = VFD dry contact, faulted = closed.** MCU **active-low** through opto. Map ALM / FA (or MA–MB) per drive at commission. |
| 7 | **No fault-reset output** in v0.1. |
| 8 | Cooler-only-if-pump = **software only** (default off). |
| 9 | Heartbeat = **any** GUI / API / later HA client. Timeout **15 s**. |
| 10 | Failsafe **both channels 0%** + RUN open (hold/preset remain available in settings). |
| 11 | Auto mode = **temp-band** (outdoor / water). Control law is a stub; API + settings expose the bands. Not “hold last setpoint only.” |
| 12 | Isolated **RS-485 populated on the first PCB**. Modbus map is optional in software. |
| 13 | Drive family = **mix / configurable** at commission. |
| 14 | Brain = Waveshare **ESP32-S3-ETH + PoE Module (B)**. |
| 15 | Enclosure = **DIN rail box** first. |
| 16 | PoE **802.3af class 0/3**. Measure budget with **both RUN coils on**. |
| 17 | Probes: **onboard ambient**, **water in**, **water out**. Waterproof leads **~3–5 m**. |
| 18 | Auth = **single API key** (`X-Api-Key`). Empty key = open LAN. |
| 19 | **MQTT publish stub only** (no subscribe/control). |
| 20 | Flash/debug = **USB-C first**. OTA later (disabled by default). |
| 21 | Hostname / mDNS = **amped-vfd.local**. |
