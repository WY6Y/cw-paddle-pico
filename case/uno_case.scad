// WY6Y CW Paddle case, Arduino Uno R3 stand-in: Uno + one 3.5 mm jack breakout,
// wired with dupont jumpers (the Pico case is case.scad; this one doesn't touch it).
// USB-B and the DC jack out one end, the paddle jack out the other. Tall enough
// for duponts standing in the Uno's headers and the jack board's pins.
// Print both parts flat, no supports:
//   openscad -D 'part="base"' -o uno-base.stl uno_case.scad
//   openscad -D 'part="lid"'  -o uno-lid.stl  uno_case.scad
// Measure your parts and fix the values marked MEASURE; everything follows.

part     = "both";    // "base", "lid", or "both" (preview)

// Arduino Uno R3 (official drawing). X runs from the USB/DC edge, Y from the
// edge the DC jack sits nearest (power/analog header side).
uno_l    = 68.58;  uno_w = 53.34;  uno_t = 1.6;
uno_holes = [[13.97, 2.54], [15.24, 50.8], [66.04, 7.62], [66.04, 35.56]];
uno_hole_d = 3.2;
usb_y    = 38.0;      // MEASURE: USB-B centre from the DC-jack-side edge (spans ~32-44)
usb_w    = 12;  usb_h = 10.9;   // connector body above the board
usb_out  = 6.2;       // how far the connector sticks out past the board edge
dc_y     = 8.1;       // MEASURE: DC jack centre (spans ~3.6-12.6)
dc_w     = 9;   dc_h = 10.9;
standoff_h = 5;       // board above the floor (solder tails underneath)
peg_d    = 2.9;       // into the 3.2 mm mounting holes
peg_tip  = 0.8;

// Room above the board for duponts in the 8.5 mm headers plus a wire bend
dupont_clear = 28;

// Jack breakout: SmartCool "3.5mm Audio Jacks", 16 x 25, same as case.scad.
jb_l     = 25;  jb_w = 16;  jb_t = 1.6;
jb_lift  = 3;
jack_z   = 2.6;       // MEASURE: plug centre above the board's top face
jack_y   = 0;
jack_d   = 7.5;
jb_hole_x  = 15.9;
jb_hole_dy = 16 - 2 * 3.52;

snap_len = 30;  snap_r = 0.8;  snap_z = 1.5;  snap_bite = 0.4;
wall = 2;  floor_t = 2;  lid_t = 2;  clr = 0.4;  side = 3;  gap = 10;
$fn = 48;

uno_x0   = usb_out - 0.3;                // connector face just proud of the wall
uno_y0   = wall + side;
uno_z0   = floor_t + standoff_h;         // board underside
uno_top  = uno_z0 + uno_t;
inner_h  = standoff_h + uno_t + dupont_clear;

in_l     = (uno_x0 - wall) + uno_l + gap + jb_l + clr;
in_w     = uno_w + 2 * side;
out_l    = in_l + 2 * wall;
out_w    = in_w + 2 * wall;
out_h    = floor_t + inner_h;

slot_w   = jb_w + 2 * clr;
jb_x0    = out_l - wall - jb_l;          // board front edge against the jack wall
jb_cy    = out_w / 2;
jb_top   = floor_t + jb_lift + jb_t;

module base() {
    difference() {
        cube([out_l, out_w, out_h]);
        translate([wall, wall, floor_t]) cube([in_l, in_w, inner_h + 1]);
        for (y = [wall, out_w - wall])
            translate([out_l / 2 - snap_len / 2 - 1, y, out_h - snap_z])
                rotate([0, 90, 0]) cylinder(r = snap_r + 0.15, h = snap_len + 2, $fn = 24);
        // USB-B: the connector body passes through the wall
        translate([-1, uno_y0 + usb_y - (usb_w + 1.5) / 2, uno_top - 0.5])
            cube([wall + 2, usb_w + 1.5, usb_h + 1.5]);
        // DC jack (no adapter today, but the plug can reach it)
        translate([-1, uno_y0 + dc_y - (dc_w + 2) / 2, uno_top - 0.5])
            cube([wall + 2, dc_w + 2, dc_h + 1.5]);
        // paddle jack
        translate([out_l - wall - 1, jb_cy + jack_y, jb_top + jack_z])
            rotate([0, 90, 0]) cylinder(d = jack_d, h = wall + 2);
    }
    // Uno standoffs with cone-tipped pegs into the mounting holes
    for (h = uno_holes)
        translate([uno_x0 + h[0], uno_y0 + h[1], floor_t - 0.01]) {
            cylinder(d = 5.5, h = standoff_h);
            cylinder(d = peg_d, h = standoff_h + uno_t + 1.5 - peg_tip);
            translate([0, 0, standoff_h + uno_t + 1.5 - peg_tip]) cylinder(d1 = peg_d, d2 = 1.8, h = peg_tip);
        }
    // jack board: ledges under the long edges, side guides, a stop behind
    for (sy = [-1, 1]) {
        for (lx = [jb_x0, out_l - wall - 4])
            translate([lx, jb_cy + sy * (slot_w / 2 - 1) - 1, floor_t - 0.01])
                cube([4, 2, jb_lift]);
        translate([jb_x0 + 4, jb_cy + (sy < 0 ? -slot_w / 2 - 1.2 : slot_w / 2), floor_t - 0.01])
            cube([jb_l - 8, 1.2, jb_lift + jb_t + 2]);
    }
    translate([jb_x0 - 2 - clr, jb_cy - slot_w / 2 + 2, floor_t - 0.01])
        cube([2, slot_w - 4, jb_lift + jb_t + 2]);
}

module lid() {
    lip = 2.5;
    difference() {
        union() {
            cube([out_l, out_w, lid_t]);
            translate([wall + clr, wall + clr, lid_t - 0.01])
                difference() {
                    cube([in_l - 2 * clr, in_w - 2 * clr, lip]);
                    translate([1.2, 1.2, -1]) cube([in_l - 2 * clr - 2.4, in_w - 2 * clr - 2.4, lip + 2]);
                }
            for (y = [wall + clr, out_w - wall - clr])
                translate([out_l / 2 - snap_len / 2, y + (y < out_w / 2 ? 1 : -1) * (snap_r - snap_bite - clr),
                           lid_t + snap_z])
                    rotate([0, 90, 0]) cylinder(r = snap_r, h = snap_len, $fn = 24);
        }
        // label, debossed in the outside face (reads right way up once flipped)
        translate([out_l / 2, out_w / 2, -0.01]) linear_extrude(0.6)
            mirror([0, 1, 0]) text("WY6Y CW", size = 9, halign = "center", valign = "center",
                                   font = "Liberation Sans:style=Bold");
    }
}

// Hold-downs from the lid, each stopping 1 mm short so the lid always closes:
// one onto the Uno's middle mounting hole (clear of every header) and two
// beside the jack board's holes (clear of the jack body and its pin header).
module lid_posts() {
    h = uno_holes[3];
    translate([uno_x0 + h[0], out_w - (uno_y0 + h[1]), lid_t - 0.3])
        cylinder(d = 6, h = out_h - uno_top - 1 + 0.3);
    for (sy = [-1, 1])
        translate([out_l - wall - jb_hole_x, out_w - (jb_cy + sy * jb_hole_dy / 2), lid_t - 0.3])
            cylinder(d = 5, h = out_h - jb_top - 1 + 0.3);
}

module lid_full() { lid(); lid_posts(); }

if (part == "base") base();
else if (part == "lid") lid_full();
else { base(); translate([0, out_w + 10, 0]) lid_full(); }
