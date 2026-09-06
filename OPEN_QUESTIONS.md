# Open questions for Josh

Amped Fabrication, Wenatchee WA. Locked v0.1 is in `docs/`. These are the decisions that still change hardware or the first field flash.

## Locked (do not re-open)

1. **4–20 mA = companion current DAC (Option B).** GP8413 @ `0x58` is 0–10 V only (VOUT0 = pump, VOUT1 = cooler). Loop current is a GP8313 / GP8600-class I2C DAC: `0x59` = pump (or dual IOUT0/IOUT1), `0x5A` = cooler when using two 1-ch parts. **Not** a V/I transmitter from GP8413 voltage.
5. **RUN = dry contact into VFD FWD–COM, active-high.** GPIO high → MOSFET → 5 V coil → NO contacts close FWD to COM. Coil de-energized or power loss = contacts open = stop.

## Analog

2. One GP8413 for both VFDs (current lock: VOUT0 = pump, VOUT1 = cooler) or one DAC per drive?
3. At STOP, should analog drop to 0 V / 4 mA or hold the last speed while RUN opens?
4. Confirm 4 mA = 0% (standard) vs 0–20 mA on these drives.

## VFD digital

6. Fault: which terminals (ALM / FA / MA-MB) and is the VFD side a dry contact or 24 V? Active when faulted — we assumed opto, MCU active-low.
7. Any “reset fault” output needed in v0.1?

## Interlock and failsafe

8. Cooler-only-if-pump: **software only** (implemented) or also a hardwired series contact on cooler RUN?
9. Heartbeat source in the shop — browser GUI, a PLC, Home Assistant, or all of the above? Timeout default is 15 s.
10. Default failsafe per channel: hold / 0% / preset? Pump vs cooler may differ (cooler 0%, pump hold?).

## Auto mode and Modbus

11. What should **auto** actually follow in v0.1 — temp band, a remote setpoint, or leave it as “holds last auto speed” (current stub)?
12. Is isolated RS-485 needed on the first build, or ship analog+RUN only?
13. Drive family / register map if Modbus is in (Yaskawa V1000, ABB ACS, PowerFlex, cheap Chinese, mix)?

## Mechanical / power

14. Confirm SKU: ESP32-S3-ETH + PoE Module (B) vs a different Waveshare PoE S3?
15. Enclosure: DIN rail box in the shop, or a custom carrier PCB this revision?
16. PoE class / switch — class 0/3 (~13 W) enough if both relays sit pulled in?
17. Waterproof probe roles (process / glycol / ambient / panel) and cable length?

## Software / ops

18. API key only, or do you want basic users later? (stub is a single `X-Api-Key`, empty key = open LAN)
19. MQTT broker on site? Topic prefix? (publish stub is ready, no subscribe/control yet)
20. OTA over Ethernet required for first flash, or USB-C only until it is stable?
21. Company-facing hostname / mDNS (`amped-vfd.local`)?

Reply in-line or file issues; firmware defaults are conservative (0% analog at boot, failsafe **zero**, interlock **off** until you flip it in Settings).
