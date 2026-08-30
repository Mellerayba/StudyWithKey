# StudyWithKey

A custom Bluetooth LE macro remote for flashcard/study-app navigation —
built from a breadboard proof-of-concept through to a battery-powered
custom PCB.

## Status
 In progress — Prototype 2 hardware ordered and shipped. Enclosure
design in progress. Firmware for Prototype 2 not yet started.

## Roadmap
- [x] Prototype 1 — breadboard BLE HID proof of concept
- [x] Prototype 2 — custom PCB, USB-C + battery power, hardware complete, DRC clean, ordered
- [ ] Prototype 2 — enclosure (parametric CadQuery model, pending fit check against fabricated board)
- [ ] Prototype 2 — firmware (FSM-based button handling, BLE HID)
- [ ] Prototype 3 — ULP RISC-V coprocessor deep-sleep refactor

## Prototypes
- [Prototype 1: Breadboard](./Physical-Prototypes/README.md) — early proof that BLE HID pairing and keypresses work end to end
- [Prototype 2: Custom PCB](./prototype-2-custom-pcb/) — purpose-built board with USB-C, LiPo charging, and a real power path

