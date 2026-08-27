"""StudyWithKey enclosure v4.

v3 -> v4:
  - Screw bosses REMOVED: geometrically impossible (PCB fills cavity, no
    mounting holes). Replaced with a snap-fit lid.
  - Retaining lip was a solid slab that would have crushed the buttons;
    now a perimeter ring.
  - J2 height added to the stack-up - it is likely the tallest part.
"""
import cadquery as cq

# ---- EXACT (STEP) ----
BOARD_W, BOARD_H, BOARD_T = 60.50, 59.50, 1.60
ESP32_H = 3.10

# ---- CONFIRMED (KiCad) ----
BUTTON_H = 4.30
MACRO_BUTTONS = [(53.50, 44.00), (53.50, 31.00), (53.50, 18.75), (53.50, 6.50)]
UTILITY_BUTTON = (45.50, 52.50)     # SW1 -> IO0 = BOOT, not reset
LED_POS = (18.50, 10.37)
J2_U, J2_V, SW6_U, SW6_V, USBC_V = 5.50, 7.50, 3.00, 50.50, 26.87

# ---- VERIFY: none of these were in the STEP ----
J2_H    = 5.80   # JST-PH side-entry body height - probably the tallest part
SW6_H   = 4.00   # ALPS SSSS912000 height above PCB
USBC_HT = 3.20   # USB-C receptacle body height

SW6_SLOT_L, SW6_SLOT_W = 9.00, 3.60
USBC_OPEN_W, USBC_OPEN_H     = 9.60, 4.20
USBC_POCKET_W, USBC_POCKET_H = 14.00, 8.20
USBC_WALL_REMAIN = 1.00

# ---- CASE ----
WALL, FLOOR, LID_T = 2.2, 2.0, 2.2
FIT, BATT_CAVITY, HEADROOM, LEDGE = 0.40, 6.0, 0.60, 1.50
CORNER_R, EDGE_R_BASE, EDGE_R_LID = 8.0, 2.5, 1.8
MACRO_HOLE_D, UTIL_HOLE_D, LED_HOLE_D = 6.00, 3.40, 3.00
CABLE_CH_W, CABLE_WALL_CUT = 8.00, 1.20
LIP_H, LIP_W, LIP_GAP = 2.0, 2.0, 0.30
BARB_L, BARB_P, BARB_H = 7.0, 0.65, 1.0     # length / protrusion / height
GROOVE_EXTRA = 0.25
LOGO_TEXT, LOGO_SIZE, LOGO_DEPTH = "StudyWithKey", 5.0, 0.5
LOGO_POS, LOGO_FONT = (25.0, 30.0), "DejaVu Sans"

INNER_W, INNER_H = BOARD_W + 2*FIT, BOARD_H + 2*FIT
OUTER_W, OUTER_H = INNER_W + 2*WALL, INNER_H + 2*WALL
PCB_BOTTOM_Z = FLOOR + BATT_CAVITY
PCB_TOP_Z    = PCB_BOTTOM_Z + BOARD_T
TALLEST      = max(ESP32_H, BUTTON_H, USBC_HT, SW6_H, J2_H)
BASE_H       = PCB_TOP_Z + TALLEST + HEADROOM
TOTAL_H      = BASE_H + LID_T
BARB_Z       = BASE_H - LIP_H + 0.4          # barb sits near the lip's bottom

bx = lambda u: WALL + FIT + u
by = lambda v: WALL + FIT + v

# barb positions: (wall, along-coordinate) chosen to miss every other feature
BARBS = [("L", by(45.0)), ("R", by(25.0)), ("B", bx(30.0)), ("T", bx(30.0))]

def _barb_box(side, pos, prot, length, height, z):
    if side in ("L", "R"):
        x = WALL - prot if side == "L" else OUTER_W - WALL
        return cq.Workplane("XY").box(prot, length, height, centered=False) \
                 .translate((x, pos - length/2, z))
    y = WALL - prot if side == "B" else OUTER_H - WALL
    return cq.Workplane("XY").box(length, prot, height, centered=False) \
             .translate((pos - length/2, y, z))

def make_base():
    b = (cq.Workplane("XY").box(OUTER_W, OUTER_H, BASE_H, centered=False)
         .edges("|Z").fillet(CORNER_R).edges("<Z").fillet(EDGE_R_BASE))
    b = b.cut(cq.Workplane("XY")
              .box(INNER_W, INNER_H, BASE_H-PCB_BOTTOM_Z+1, centered=False)
              .translate((WALL, WALL, PCB_BOTTOM_Z)))
    b = b.cut(cq.Workplane("XY")
              .box(INNER_W-2*LEDGE, INNER_H-2*LEDGE, BATT_CAVITY, centered=False)
              .translate((WALL+LEDGE, WALL+LEDGE, FLOOR)))
    # USB-C: outer recess for the plug overmould, then through-slot for the tip
    b = b.cut(cq.Workplane("YZ").center(by(USBC_V), PCB_TOP_Z + USBC_HT/2)
              .slot2D(USBC_POCKET_W, USBC_POCKET_H, 0)
              .extrude(WALL+FIT-USBC_WALL_REMAIN+1).translate((-1,0,0)))
    b = b.cut(cq.Workplane("YZ").center(by(USBC_V), PCB_TOP_Z + USBC_HT/2)
              .slot2D(USBC_OPEN_W, USBC_OPEN_H, 0)
              .extrude(WALL+FIT+3).translate((-1,0,0)))
    # battery cable channel: J2 on top of the PCB down into the bay
    b = b.cut(cq.Workplane("XY")
              .box(CABLE_CH_W, WALL+LEDGE-(WALL-CABLE_WALL_CUT), PCB_TOP_Z-FLOOR,
                   centered=False)
              .translate((bx(J2_U)-CABLE_CH_W/2, WALL-CABLE_WALL_CUT, FLOOR)))
    # snap grooves in the inner wall
    for side, pos in BARBS:
        b = b.cut(_barb_box(side, pos, BARB_P+GROOVE_EXTRA,
                            BARB_L+1.0, BARB_H+0.5, BARB_Z-0.25))
    return b

def make_lid():
    l = (cq.Workplane("XY").box(OUTER_W, OUTER_H, LID_T, centered=False)
         .edges("|Z").fillet(CORNER_R).edges(">Z").fillet(EDGE_R_LID))
    # perimeter RING, not a slab - a slab would sit on the buttons
    ring = (cq.Workplane("XY")
            .box(INNER_W-LIP_GAP, INNER_H-LIP_GAP, LIP_H, centered=False)
            .translate((WALL+LIP_GAP/2, WALL+LIP_GAP/2, -LIP_H))
            .edges("|Z").fillet(CORNER_R-WALL-LIP_GAP/2))
    ring = ring.cut(cq.Workplane("XY")
                    .box(INNER_W-LIP_GAP-2*LIP_W, INNER_H-LIP_GAP-2*LIP_W,
                         LIP_H+2, centered=False)
                    .translate((WALL+LIP_GAP/2+LIP_W, WALL+LIP_GAP/2+LIP_W, -LIP_H-1)))
    l = l.union(ring)
    for side, pos in BARBS:
        l = l.union(_barb_box(side, pos, BARB_P, BARB_L, BARB_H, BARB_Z - BASE_H))
    for u,v in MACRO_BUTTONS:
        l = l.cut(cq.Workplane("XY").center(bx(u),by(v))
                  .circle(MACRO_HOLE_D/2).extrude(LID_T+6).translate((0,0,-4)))
        l = l.cut(cq.Workplane("XY").center(bx(u),by(v))
                  .circle(MACRO_HOLE_D/2+0.9).workplane(offset=0.9)
                  .circle(MACRO_HOLE_D/2).loft().translate((0,0,LID_T-0.9)))
    u,v = UTILITY_BUTTON
    l = l.cut(cq.Workplane("XY").center(bx(u),by(v))
              .circle(UTIL_HOLE_D/2).extrude(LID_T+6).translate((0,0,-4)))
    l = l.cut(cq.Workplane("XY").center(bx(LED_POS[0]),by(LED_POS[1]))
              .circle(LED_HOLE_D/2).extrude(LID_T+6).translate((0,0,-4)))
    l = l.cut(cq.Workplane("XY").center(bx(SW6_U),by(SW6_V))
              .slot2D(SW6_SLOT_L, SW6_SLOT_W, 90).extrude(LID_T+6)
              .translate((0,0,-4)))
    l = l.cut(cq.Workplane("XY").center(bx(LOGO_POS[0]), by(LOGO_POS[1]))
              .text(LOGO_TEXT, LOGO_SIZE, 1.0, combine=False, font=LOGO_FONT)
              .translate((0,0,LID_T-LOGO_DEPTH)))
    return l

if __name__ == "__main__":
    base, lid = make_base(), make_lid()
    for o,n in [(base,"case_base"),(lid,"case_lid")]:
        cq.exporters.export(o, f"/mnt/user-data/outputs/{n}.step")
        cq.exporters.export(o, f"/mnt/user-data/outputs/{n}.stl")
    print(f"External   {OUTER_W:.2f} x {OUTER_H:.2f} x {TOTAL_H:.2f} mm")
    print(f"Tallest    {TALLEST:.2f}mm (J2={J2_H}, buttons={BUTTON_H}, SW6={SW6_H})")
    print(f"Lid ring   bottom Z={BASE_H-LIP_H:.2f}, tallest part top Z={PCB_TOP_Z+TALLEST:.2f}")
    print(f"Retention  4 snap barbs, no screws")
