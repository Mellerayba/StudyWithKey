# Firmware

C firmware for the StudyWithKey remote, running on the ESP32-S3-WROOM-1.
Built from a blank ESP-IDF project, staged deliberately so each capability
was proven working in isolation before being combined with the next.

## Status

Core functionality complete and working on hardware.
After power measurements, I found a negligible difference between the built in
C deep sleep module, versus a RISCV deep sleep example code. Therefore, I decided
that it was best to reduce complexity, and not move forward with the RISCV deep 
sleep idea.

## What it does

- Reads 5 physical buttons (4 macro keys + 1 utility) with real
  timestamp-based debouncing, not naive polling
- Presents as a BLE HID keyboard, paired and tested against macOS
- Sends real keypresses: Left Arrow, Right Arrow, Space, H
- Deep sleep after a period of inactivity, waking on button press
- A long-press utility button that clears stored BLE bonding, for
  recovering from stale-pairing issues without a full reflash

## Pin mapping

| Physical pin (WROOM-1) | GPIO | Function |
|---|---|---|
| 27 | GPIO0 | SW1 — long-press: clear BLE bonds and re-advertise |
| 5 | GPIO1 | SW3 — Left Arrow; also the deep-sleep wake source |
| 35 | GPIO42 | SW2 — H |
| 33 | GPIO40 | SW4 — Space |
| 31 | GPIO38 | SW5 — Right Arrow |

GPIO0 and GPIO1 are the only two RTC-capable pins on the ESP32-S3
(GPIO0–21), which is why the wake source specifically has to live on
Left rather than any of the other three macro keys — a hardware
limitation, not a design choice.

## Architecture

Plain C, single source file (`studywithkey-fw.c`) plus the Bluedroid BLE
HID profile files (`hid_dev.c`, `esp_hidd_prf_api.c`, `hid_device_le_prf.c`
and headers), adapted from ESP-IDF's official
`examples/bluetooth/bluedroid/ble/ble_hid_device_demo`.

Each button is tracked with its own pair of state variables — last
accepted level, timestamp of last accepted edge — checked independently
per loop pass. No interrupts; a single polling loop with a short
`vTaskDelay` paces all five checks together.

## Build stages (how this was actually built)

Built and proven in order, each stage verified working before the next
was layered on — deliberately, so any bug found belonged to exactly one
new piece of code, not a tangle of everything at once.

1. **Toolchain + board bring-up** — minimal blank project, confirmed the
   board boots and flashes correctly before writing any real logic.
2. **Single GPIO read, no debouncing** — deliberately naive, to see real
   mechanical switch bounce firsthand before building the fix for it.
3. **Real debouncing** — timestamp-gated edge detection
   (`esp_timer_get_time()`-based), not just a polling delay.
4. **BLE HID pairing proven standalone** — official Bluedroid demo copied
   and run unmodified first, to confirm pairing worked on this hardware
   before any custom code touched it.
5. **All 4 macro buttons wired to real keypresses**, debounce logic
   generalized from one button to four independent trackers.
6. **Deep sleep + wake-on-button**, plus a long-press rebonding utility
   button.
7. **(In progress) ULP-RISC-V** — see below.

## Debugging notes worth keeping

A few bugs found along the way were subtle enough, or generally-useful
enough, to be worth recording rather than just fixing quietly:

- **Floating GPIOs.** Extending debounce from one button to four, the
  `gpio_config()` pin mask was never updated to include the three new
  pins — they were completely unconfigured, no pull-up, genuinely
  floating. Symptom was dramatic (button inputs "spamming" the moment a
  wire touched the pin, with no press involved) but the cause was a
  one-line oversight, not a logic bug.
- **`vTaskDelay` rounding to zero ticks.** FreeRTOS's default tick rate
  is 100Hz (10ms/tick). `pdMS_TO_TICKS(5)` truncates to 0 ticks at that
  rate, meaning the requested "5ms delay" didn't delay at all — the loop
  spun fast enough to starve the idle task and trip the watchdog. Fixed
  by using a delay ≥10ms, since sub-tick delays silently round down
  rather than erroring.
- **Stale BLE bonding after reflashing.** Repeated reflashing during
  development regenerates the device's BLE identity, but macOS keeps the
  previous pairing's keys cached under the same name — producing
  connects that authenticate but then get `GATT_WRITE_NOT_PERMIT`
  errors on notification subscription, with no actual keypresses
  arriving. Fixed by forgetting the device on the Mac and re-pairing
  fresh after every reflash during development; this is also the
  motivating use case for the SW1 rebonding button in the shipped
  firmware.
- **CMake `PRIV_REQUIRES` and `sdkconfig.defaults`.** Merging Bluedroid
  code into a project created via `idf.py create-project` isn't just a
  matter of adding source files — the project's default `sdkconfig` has
  Bluetooth disabled entirely, and no `PRIV_REQUIRES` line can expose
  headers a component hasn't itself enabled. Fixed by copying the real
  `sdkconfig.defaults` from the working example rather than hand-picking
  menuconfig options.

## Open items

- **ULP-RISC-V (cancelled).** The plan is not to replace the
  working `ext0` hardware wake outright, but to evaluate whether a
  ULP-RISC-V debounce stage — confirming a press is sustained before
  waking the main CPU — is worth its own small power cost by preventing
  false full-system wakes from electrical noise. This is being measured,
  not assumed; the honest goal is a repo that says which one actually
  won, not just "used RISC-V because it's impressive."

## Files

- `studywithkey-fw.c` — main application logic
- `hid_dev.c` / `hid_dev.h` — HID report building
- `esp_hidd_prf_api.c` / `esp_hidd_prf_api.h` — HID device profile API
- `hid_device_le_prf.c` / `hidd_le_prf_int.h` — BLE GATT service
  implementation for the HID profile
- `CMakeLists.txt` — component registration, `PRIV_REQUIRES bt nvs_flash
  esp_driver_gpio`
- `sdkconfig.defaults` — Bluetooth/Bluedroid enabled by default
