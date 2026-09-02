# StudyWithKey

A custom Bluetooth LE macro remote for flashcard/study-app navigation —
built from a breadboard proof-of-concept through to a battery-powered
custom PCB.

## Status
Currently halted - critical flaw in the ordered PCBs and I am not in a financial position currently to reorder them.
Firmware is completely functional and available to use freely

## Roadmap
- [x] Prototype 1 — breadboard BLE HID proof of concept
- [x] Prototype 2 — custom PCB, USB-C + battery power, hardware complete, DRC clean, ordered
- [ ] Prototype 2 — enclosure (parametric CadQuery model, pending fit check against fabricated board)
- [x] Prototype 2 — firmware (FSM-based button handling, BLE HID)
- [ ] Prototype 3 — ULP RISC-V coprocessor deep-sleep refactor

## Roadmap Update
Sadly the ordered PCB's arrived with a fatal flaw, the switches were miss oriented - causing an internal short circuit. I managed to re - solder some switches 
however it is not viable to do this for the entire order. I am not currently in the financial position to continue this project
after such a set back; however, the firmware is completely functional and available to use on any ESP-32.

## Prototypes
- [Prototype 1: Breadboard](./Physical-Prototypes/README.md) — early proof that BLE HID pairing and keypresses work end to end
- [Prototype 2: Custom PCB](./prototype-2-custom-pcb/) — purpose-built board with USB-C, LiPo charging, and a real power path
- [Physical PCB: Resoldered demo](././Physical-Prototypes/PhysicalProduct.md) — Demo photos and videos of the real custom PCB, showing that the firmware works
