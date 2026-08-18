# Physical Prototype 1 — Breadboard Proof of Concept

## What this was
An ESP32 dev board on a breadboard, running modified example BLE HID 
keyboard firmware, wired to a handful of push buttons.

## What it proved
- BLE HID pairing and keypress delivery work end to end on this hardware
- Validated the core concept before investing in a custom PCB

## Limitations
- Firmware was example code with trivial button-to-keypress mapping, 
  no debouncing or state machine
- No power management — always USB-powered, no battery/sleep
- No custom hardware — off-the-shelf board only

This prototype's job was to answer one question — "does this concept 
work at all?" — before committing time to hardware design. It did, 
which is what justified moving to Prototype 2.
