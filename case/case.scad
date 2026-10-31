// WY6Y CW Paddle case: Pico 2 + 1 or 2 3.5 mm jack breakout boards.
// USB out one end, jacks out the other. Print both parts flat, no supports:
//   openscad -D 'part="base"' -o base.stl case.scad
//   openscad -D 'part="lid"'  -o lid.stl  case.scad
// Measure your parts and fix the values marked MEASURE; everything follows.

part     = "both";    // "base", "lid", or "both" (preview)
n_jacks  = 1;         // 1 or 2 breakout boards side by side

// Pico 2 H (datasheet)
pico_l   = 51;  pico_w = 21;  pico_t = 1.0;
hole_dx  = 47;  hole_dy = 11.4;
// Pico H: factory headers point down. The plastic strips rest on two rails
// and the pin tips drop into slots in them; lid pegs go into the mounting holes.
row_dy   = 17.78;     // header rows, 0.7" apart
hdr_t    = 2.5;       // MEASURE: header plastic thickness under the board
pin_len  = 6;         // MEASURE: pin length below the plastic
rail_h   = pin_len + 1.5;
peg_d    = 1.6;       // into the 2.1 mm mounting holes
usb_w    = 12;  usb_h = 8;   // opening sized for the cable's moulded plug

// Jack breakout (Amazon: 16 x 25). MEASURE the rest.
jb_l     = 25;        // along the plug axis
jb_w     = 16;
jb_t     = 1.6;
jb_lift  = 3;         // board sits on ledges this high (solder tails below)
jack_z   = 2.6;       // MEASURE: plug centre above the board's top face
jack_y   = 0;         // MEASURE: plug centre offset from the board's centreline
jack_d   = 6.5;       // plug hole; a plug shank is 3.5, its collar ~6

// LED window: thin skin in the lid that glows when a paddle is closed
led_x    = 6;   led_y = -4.5;   // from the Pico's USB edge / centreline (approx)
led_d    = 6;   led_skin = 0.6;

wall = 2;  floor_t = 2;  lid_t = 2;  clr = 0.4;  gap = 6;  inner_h = 17;
$fn = 48;

slot_w   = jb_w + 2 * clr;
jacks_w  = n_jacks * slot_w + (n_jacks - 1) * 3;
in_l     = 0.5 + pico_l + gap + jb_l + clr;
in_w     = max(pico_w + 2 * clr, jacks_w) + 2;
out_l    = in_l + 2 * wall;
out_w    = in_w + 2 * wall;
out_h    = floor_t + inner_h;

pico_x0  = wall + 0.5;                  // Pico's USB edge nearly touches the wall
jb_x0    = out_l - wall - jb_l;         // board front edge against the jack wall
pico_top = floor_t + rail_h + hdr_t + pico_t;
jb_top   = floor_t + jb_lift + jb_t;

function jack_cy(i) = out_w / 2 - jacks_w / 2 + slot_w / 2 + i * (slot_w + 3);

module base() {
    difference() {
        cube([out_l, out_w, out_h]);
        translate([wall, wall, floor_t]) cube([in_l, in_w, inner_h + 1]);
        // USB
        translate([-1, out_w / 2 - usb_w / 2, pico_top - 1])
            cube([wall + 2, usb_w, usb_h]);
        // jacks
        for (i = [0 : n_jacks - 1])
            translate([out_l - wall - 1, jack_cy(i) + jack_y, jb_top + jack_z])
                rotate([0, 90, 0]) cylinder(d = jack_d, h = wall + 2);
    }
    // Pico rails: header plastic sits on top, pins drop into the slot
    for (sy = [-1, 1])
        translate([pico_x0, out_w / 2 + sy * row_dy / 2 - 2, floor_t - 0.01])
            difference() {
                cube([pico_l, 4, rail_h]);
                translate([-1, 2 - 0.9, 1]) cube([pico_l + 2, 1.8, rail_h]);
            }
    // end stop: takes the push when the USB cable goes in
    translate([pico_x0 + pico_l + clr, out_w / 2 - 6, floor_t - 0.01])
        cube([1.5, 12, pico_top - floor_t + 1]);
    // jack boards: corner ledges underneath, side guides, a stop behind
    for (i = [0 : n_jacks - 1]) {
        cy = jack_cy(i);
        for (sy = [-1, 1]) {
            // ledges under the board edges, front and back
            for (lx = [jb_x0, out_l - wall - 4])
                translate([lx, cy + sy * (slot_w / 2 - 1.5) - 1.5, floor_t - 0.01])
                    cube([4, 3, jb_lift]);
            // guides beside the board
            translate([jb_x0 + 4, cy + (sy < 0 ? -slot_w / 2 - 1.2 : slot_w / 2), floor_t - 0.01])
                cube([jb_l - 8, 1.2, jb_lift + jb_t + 2]);
        }
        // back stop: takes the push when a plug goes in
        translate([jb_x0 - 2 - clr, cy - slot_w / 2 + 2, floor_t - 0.01])
            cube([2, slot_w - 4, jb_lift + jb_t + 2]);
    }
}

module lid() {
    lip = 2.5;
    difference() {
        union() {
            cube([out_l, out_w, lid_t]);
            // press-fit lip inside the walls
            translate([wall + clr, wall + clr, lid_t - 0.01])
                difference() {
                    cube([in_l - 2 * clr, in_w - 2 * clr, lip]);
                    translate([1.2, 1.2, -1]) cube([in_l - 2 * clr - 2.4, in_w - 2 * clr - 2.4, lip + 2]);
                }
            // ribs that press each jack board down onto its ledges
            for (i = [0 : n_jacks - 1])
                for (sy = [-1, 1])
                    translate([jb_x0 + 6, out_w - jack_cy(i) + sy * (slot_w / 2 - 1.5) - 1, lid_t - 0.01])
                        cube([jb_l - 12, 2, out_h - jb_top - clr]);
        }
        // gap in the lip where the USB plug comes in
        translate([wall - 1, out_w / 2 - usb_w / 2, lid_t - 0.02]) cube([3, usb_w, 4]);
        // LED window (lid is printed upside down, so mirror Y)
        translate([pico_x0 + led_x, out_w - (out_w / 2 + led_y), led_skin])
            cylinder(d = led_d, h = lid_t);
        // label, debossed in the outside face (reads right way up once flipped)
        translate([out_l / 2, out_w / 2, -0.01]) linear_extrude(0.6)
            mirror([0, 1, 0]) text("WY6Y CW", size = 6, halign = "center", valign = "center",
                                   font = "Liberation Sans:style=Bold");
    }
}

module lid_pegs() {
    post = out_h - pico_top - clr;          // lid underside down to the Pico top
    for (sx = [-1, 1], sy = [-1, 1])
        translate([pico_x0 + pico_l / 2 + sx * hole_dx / 2, out_w - (out_w / 2 + sy * hole_dy / 2), lid_t - 0.01]) {
            cylinder(d = 4, h = post);
            cylinder(d = peg_d, h = post + 1.5);
        }
}

module lid_full() { lid(); lid_pegs(); }

if (part == "base") base();
else if (part == "lid") lid_full();
else { base(); translate([0, out_w + 10, 0]) lid_full(); }
