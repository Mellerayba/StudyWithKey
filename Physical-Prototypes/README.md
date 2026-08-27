# Physical Prototype 1 — Breadboard Proof of Concept
![Breadboard](./Physical-Prototypes/Breadboard.jpeg)
## What this was
An ESP32 dev board on a breadboard, running modified example BLE HID
keyboard firmware, wired to a handful of push buttons.

## What it proved
- BLE HID pairing and keypress delivery work end to end on this hardware
- Validated the core concept before investing in a custom PCB

## Limitations (by design — this was a spike, not a product)
- Firmware was example code with trivial button-to-keypress mapping,
  no debouncing or state machine
- No power management — always USB-powered, no battery/sleep
- No custom hardware — off-the-shelf dev board only

This prototype's job was to answer one question — "does this concept
work at all?" — before committing time to hardware design. It did,
which is what justified moving to Prototype 2.


# Enclosure - CAD Design

## Case Prototype 1

Parametric, code-generated case for the Prototype 2 PCB — two-part
snap-fit shell, generated with [CadQuery](https://cadquery.readthedocs.io/)
from real board geometry rather than hand-modeled from a datasheet.

## Files

| File | What it is |
|---|---|
| `case_gen.py` | The generator. Every dimension is a named constant at the top — edit and re-run to regenerate both STEP and STL. |
| `case_base.step` / `.stl` | Bottom shell: battery cavity, USB-C cutout, cable channel, snap grooves. |
| `case_lid.step` / `.stl` | Top shell: button holes, slide-switch slot, charge LED hole, engraved wordmark, snap barbs. |

STEP files are for further editing (FreeCAD, Fusion 360). STL files are
print-ready as generated.

## Design notes

- **Board geometry (exact):** pulled directly from the KiCad STEP export
  rather than measured/guessed. Board outline,ESP32 footprint, 
  and all passive heights came from this.
- **Connector/switch/button positions:** the STEP export omits any
  footprint without a bundled 3D model — this includes J1 (USB-C), J2
  (battery connector), and all six switches, since they come from
  easyeda2kicad/generic connector libraries with no STEP model attached.
  Their positions were taken from KiCad's Footprint Properties panel
  instead and converted from mils to the board coordinate
  system used in this script.
- **Retention is snap-fit, not screws.** The PCB has no mounting holes and
  fills its cavity edge-to-edge, so there is no position for a corner
  screw boss that doesn't either sit on top of the board or fall outside
  the case wall — verified by checking the boss radius against the
  available clearance at each corner. Four snap barbs on the lid's
  retaining ring engage grooves in the base wall instead.
- **USB-C cutout is two-stage:** an outer recess sized to the plug's
  moulded housing, then a smaller through-slot for the plug tip. A single
  flush rectangular hole would have left the wall too
  thick for a plug to seat or latch.
- **SW6 (slide switch) opening is in the lid, not the side wall** — the
  ALPS SSSS912000 is top-actuated, so the case has to be actuated from
  above, not from the edge.
- **Height stack-up is set by J2, not the buttons.** The JST-PH connector
  stands taller than the 6×6×4.3mm tactile switches, so it determines the
  internal clearance and overall case height.
