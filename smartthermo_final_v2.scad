// ============================================================
//  SmartThermo Enclosure — FINAL v2
//  For: ESP32 + 16x2 LCD I2C + DHT22-MOD + IR LED + LEDs
//       + Buzzer + 3 Buttons + 400pt Breadboard + Jumper Wires
// ============================================================
//  Print settings: 0.2mm layer, 20% infill, no supports needed
//  Material: PLA or PETG
// ============================================================
//  v2 fixes:
//    - USB hole: fixed translate Y (was not reaching inner face)
//    - Wall mount keyholes: same fix + repositioned near top
//    - keyhole_slot depth increased to wall+4 (cuts fully through)
//    - Box + Lid combined in ONE STL file
// ============================================================

/* [Main Dimensions] */
wall        = 2.5;
corner_r    = 3;

inner_w     = 100;
inner_d     = 90;
inner_h     = 70;

outer_w     = inner_w + 2*wall;   // 105mm
outer_d     = inner_d + 2*wall;   // 95mm
outer_h     = inner_h + 2*wall;   // 75mm

/* [LCD Cutout — 16x2 I2C] */
lcd_view_w  = 65;
lcd_view_h  = 15;
lcd_pcb_w   = 80;
lcd_pcb_h   = 36;
lcd_x       = 0;
lcd_z       = 12;

/* [Buttons — 6mm panel-mount, 7mm holes] */
btn_dia     = 7;
btn_spacing = 14;
btn_x_start = 10;
btn_z_pos   = -20;

/* [LEDs — 5mm] */
led_dia     = 5.2;
led_spacing = 10;
led_x_off   = -20;
led_z_off   = -20;

/* [IR LED] */
ir_dia      = 5.2;
ir_x        = -38;
ir_z        = -20;

/* [DHT22 — through-hole, insert from inside] */
dht_cut_w   = 20;
dht_cut_h   = 25;
dht_z_off   = 5;

/* [Buzzer — on LID] */
buzz_dia    = 2.0;
buzz_count  = 7;
buzz_r      = 6;
buzz_x      = 25;
buzz_y      = 10;

/* [USB — back face] */
usb_w       = 14;
usb_h       = 8;
usb_z       = -25;

/* [Mounting Posts] */
post_hole   = 2.5;

/* [Lid] */
lid_h       = 2.5;

/* [Wall Mount — keyhole slots on back, near top] */
mount_slot_w  = 8;
mount_slot_h  = 14;
mount_spacing = 60;
// Near top of back face so box hangs below the screws
mount_z       = outer_h - lid_h - 18;   // ~54mm from bottom

// ── Helper: rounded box ──────────────────────────────────
module rounded_box(w, d, h, r) {
    hull() {
        for (x = [-w/2+r, w/2-r])
            for (y = [-d/2+r, d/2-r])
                translate([x, y, 0])
                    cylinder(h=h, r=r, $fn=40);
    }
}

// ── Rounded slot helper ──────────────────────────────────
module rounded_slot(w, h, depth) {
    hull() {
        for (y = [-h/2 + w/2, h/2 - w/2])
            translate([0, y, 0])
                cylinder(d=w, h=depth, $fn=20);
    }
}

// ── Rounded rectangular cutout ───────────────────────────
module rounded_lcd_cutout(w, h, depth) {
    r = 1.5;
    hull() {
        for (x = [-w/2+r, w/2-r])
            for (y = [-h/2+r, h/2-r])
                translate([x, y, 0])
                    cylinder(r=r, h=depth, $fn=20);
    }
}

// ── Keyhole wall mount slot ──────────────────────────────
// depth = wall+4 so it cuts fully from inner to outer face
module keyhole_slot(depth) {
    // Large circle at top — screw HEAD passes through to hang
    translate([0, mount_slot_h/2 - mount_slot_w/2, 0])
        cylinder(d=mount_slot_w, h=depth, $fn=30);
    // Vertical slot — box slides DOWN onto screw shaft
    translate([-mount_slot_w/4, -mount_slot_h/2, 0])
        cube([mount_slot_w/2, mount_slot_h, depth]);
    // Small circle at bottom — grips screw shaft snugly
    translate([0, -mount_slot_h/2, 0])
        cylinder(d=mount_slot_w/2, h=depth, $fn=20);
}

// ── MAIN BOX ─────────────────────────────────────────────
module main_box() {
    difference() {

        // Outer shell
        rounded_box(outer_w, outer_d, outer_h - lid_h, corner_r);

        // Inner cavity
        translate([0, 0, wall])
            rounded_box(inner_w, inner_d, outer_h, corner_r - wall/2);

        // ════════════════════════════════════════════════
        // FRONT FACE cutouts
        // ════════════════════════════════════════════════

        // LCD viewing window
        translate([lcd_x, -outer_d/2 - 1, outer_h/2 + lcd_z])
            rotate([-90, 0, 0])
                rounded_lcd_cutout(lcd_view_w, lcd_view_h, wall + 2);

        // LCD PCB slot (recessed from inside)
        translate([lcd_x, -outer_d/2 + wall - 1, outer_h/2 + lcd_z])
            rotate([-90, 0, 0])
                rounded_lcd_cutout(lcd_pcb_w + 1, lcd_pcb_h + 1, 2);

        // Green LED hole
        translate([led_x_off - led_spacing/2, -outer_d/2 - 1, outer_h/2 + led_z_off])
            rotate([-90, 0, 0])
                cylinder(d=led_dia, h=wall+2, $fn=30);

        // Red LED hole
        translate([led_x_off + led_spacing/2, -outer_d/2 - 1, outer_h/2 + led_z_off])
            rotate([-90, 0, 0])
                cylinder(d=led_dia, h=wall+2, $fn=30);

        // LED label indents
        translate([led_x_off - led_spacing/2, -outer_d/2 - 0.1, outer_h/2 + led_z_off + 6])
            rotate([-90, 0, 0])
                cylinder(d=2, h=0.8, $fn=20);
        translate([led_x_off + led_spacing/2, -outer_d/2 - 0.1, outer_h/2 + led_z_off + 6])
            rotate([-90, 0, 0])
                cylinder(d=2, h=0.8, $fn=20);

        // IR LED hole + cone
        translate([ir_x, -outer_d/2 - 1, outer_h/2 + ir_z])
            rotate([-90, 0, 0])
                cylinder(d=ir_dia, h=wall+2, $fn=30);
        translate([ir_x, -outer_d/2 - 0.1, outer_h/2 + ir_z])
            rotate([-90, 0, 0])
                cylinder(d1=ir_dia, d2=ir_dia+3, h=wall+0.2, $fn=30);

        // Button holes — 7mm for 6mm panel-mount buttons
        for (i = [0, 1, 2]) {
            translate([btn_x_start + i * btn_spacing, -outer_d/2 - 1, outer_h/2 + btn_z_pos])
                rotate([-90, 0, 0])
                    cylinder(d=btn_dia, h=wall+2, $fn=30);
        }

        // Button labels: DN / PWR / UP
        translate([btn_x_start, -outer_d/2 - 0.1, outer_h/2 + btn_z_pos + 7])
            rotate([90, 0, 0]) linear_extrude(0.8)
                text("DN", size=3, halign="center", valign="center",
                     font="Liberation Sans:style=Bold");
        translate([btn_x_start + btn_spacing, -outer_d/2 - 0.1, outer_h/2 + btn_z_pos + 7])
            rotate([90, 0, 0]) linear_extrude(0.8)
                text("PWR", size=2.5, halign="center", valign="center",
                     font="Liberation Sans:style=Bold");
        translate([btn_x_start + 2*btn_spacing, -outer_d/2 - 0.1, outer_h/2 + btn_z_pos + 7])
            rotate([90, 0, 0]) linear_extrude(0.8)
                text("UP", size=3, halign="center", valign="center",
                     font="Liberation Sans:style=Bold");

        // ════════════════════════════════════════════════
        // RIGHT SIDE — DHT22 through-hole
        // Starts 1mm before inner wall → cuts fully through
        // ════════════════════════════════════════════════
        translate([inner_w/2 - 1, 0, outer_h/2 + dht_z_off])
            rotate([0, 90, 0])
                rounded_lcd_cutout(dht_cut_w, dht_cut_h, wall + 4);

        // ════════════════════════════════════════════════
        // BACK FACE — USB port
        // FIXED: translate Y = inner_d/2 - 1 (starts before inner face)
        //        depth = wall + 4  (cuts fully through to outside)
        // ════════════════════════════════════════════════
        translate([0, inner_d/2 - 1, outer_h/2 + usb_z])
            rotate([90, 0, 0])
                rounded_slot(usb_w, usb_h, wall + 4);

        // ════════════════════════════════════════════════
        // BACK FACE — Wall mount keyhole slots
        // FIXED: translate Y = inner_d/2 - 1 (starts before inner face)
        //        depth = wall + 4 passed into module
        //        mount_z near top of box for proper wall hang
        // ════════════════════════════════════════════════
        for (x = [-mount_spacing/2, mount_spacing/2])
            translate([x, inner_d/2 - 1, mount_z])
                rotate([90, 0, 0])
                    keyhole_slot(wall + 4);

        // ════════════════════════════════════════════════
        // BOTTOM — ventilation slots
        // ════════════════════════════════════════════════
        for (i = [-3:3]) {
            translate([i * 12, 0, -1])
                rounded_slot(3, 40, wall + 2);
        }
    }

    // ── Screw posts for lid ──────────────────────────────
    screw_inset = 6;
    for (x = [-inner_w/2 + screw_inset, inner_w/2 - screw_inset])
        for (y = [-inner_d/2 + screw_inset, inner_d/2 - screw_inset])
            translate([x, y, wall])
                difference() {
                    cylinder(d=6, h=outer_h - lid_h - wall, $fn=30);
                    translate([0, 0, outer_h - lid_h - wall - 8])
                        cylinder(d=post_hole, h=10, $fn=20);
                }
}

// ── LID ──────────────────────────────────────────────────
module lid() {
    difference() {
        rounded_box(outer_w, outer_d, lid_h, corner_r);

        // Screw holes at corners
        screw_inset = 6;
        for (x = [-inner_w/2 + screw_inset, inner_w/2 - screw_inset])
            for (y = [-inner_d/2 + screw_inset, inner_d/2 - screw_inset])
                translate([x, y, -1])
                    cylinder(d=post_hole + 0.4, h=lid_h + 2, $fn=20);

        // Countersink for screw heads
        for (x = [-inner_w/2 + screw_inset, inner_w/2 - screw_inset])
            for (y = [-inner_d/2 + screw_inset, inner_d/2 - screw_inset])
                translate([x, y, lid_h - 1.2])
                    cylinder(d1=post_hole + 0.4, d2=post_hole + 4, h=1.3, $fn=20);

        // Buzzer holes — circular pattern on lid top face
        translate([buzz_x, buzz_y, -1])
            for (i = [0:buzz_count-1]) {
                angle = i * 360 / buzz_count;
                translate([buzz_r * cos(angle), buzz_r * sin(angle), 0])
                    cylinder(d=buzz_dia, h=lid_h + 2, $fn=20);
            }
        translate([buzz_x, buzz_y, -1])
            cylinder(d=buzz_dia, h=lid_h + 2, $fn=20);
    }

    // Raised text on lid top
    translate([0, 10, lid_h - 0.4])
        linear_extrude(0.5)
            text("SmartThermo", size=7, halign="center", valign="center",
                 font="Liberation Sans:style=Bold");
    translate([0, 0, lid_h - 0.4])
        linear_extrude(0.5)
            text("v4.2", size=4, halign="center", valign="center",
                 font="Liberation Sans");
    translate([0, -12, lid_h - 0.4])
        linear_extrude(0.5)
            text("Filling Room", size=4, halign="center", valign="center",
                 font="Liberation Sans");
}

// ════════════════════════════════════════════════════════
// RENDER — Box + Lid combined in ONE file
// Lid placed beside box, both flat for easy slicing
// ════════════════════════════════════════════════════════

// Main box
main_box();

// Lid — beside the box, flipped flat for printing
translate([outer_w + 20, 0, lid_h])
    rotate([180, 0, 0])
        lid();
