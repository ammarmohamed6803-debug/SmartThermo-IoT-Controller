// ============================================================
//  SmartThermo Enclosure — 3D Printable Housing
//  For: ESP32 + 16x2 LCD I2C + DHT22 + IR LED + LEDs
//       + Buzzer + 3 Buttons + Transistor Circuit
// ============================================================
//  Print settings: 0.2mm layer, 20% infill, no supports needed
//  Material: PLA or PETG
// ============================================================

/* [Main Dimensions] */
wall        = 2.5;       // wall thickness
corner_r    = 3;         // corner radius

// Internal dimensions
inner_w     = 90;        // width (X)
inner_d     = 65;        // depth (Y)
inner_h     = 35;        // height (Z)

// Outer dimensions (computed)
outer_w     = inner_w + 2*wall;
outer_d     = inner_d + 2*wall;
outer_h     = inner_h + 2*wall;

/* [LCD Cutout — 16x2 I2C] */
// PCB: ~80x36mm, viewable: ~64x14mm
lcd_view_w  = 65;
lcd_view_h  = 15;
lcd_pcb_w   = 80;
lcd_pcb_h   = 36;
lcd_x       = 0;         // centered on front face
lcd_z       = 8;         // offset above center

/* [Buttons — 6x6mm tactile, FRONT FACE bottom-right] */
btn_dia     = 4;         // hole diameter for button caps
btn_spacing = 10;        // spacing between buttons
btn_x_start = 8;         // X start position (well clear of right corner post)
btn_z_pos   = -15;       // Z offset from center — same row as LEDs

/* [LEDs — 5mm] */
led_dia     = 5.2;       // slightly oversized for press-fit
led_spacing = 10;
led_x_off   = -12;       // X offset from center on front face (left side)
led_z_off   = -15;       // Z offset from center — well below LCD

/* [IR LED] */
ir_dia      = 5.2;
ir_x        = -28;       // moved well clear of corner screw post
ir_z        = -15;       // same row as LEDs and buttons

/* [DHT22 Sensor] */
// Vent slots on right side for airflow
dht_slot_w  = 2;
dht_slot_h  = 15;
dht_slots   = 6;
dht_slot_sp = 4;
dht_z_off   = 5;

/* [Buzzer Holes — top face] */
buzz_dia    = 1.5;
buzz_count  = 7;         // holes in circle pattern
buzz_r      = 6;         // radius of hole pattern
buzz_x      = 25;        // X offset on top face
buzz_y      = 10;

/* [USB Port — back face] */
usb_w       = 12;
usb_h       = 7;
usb_z       = -10;       // near bottom

/* [Mounting Posts] */
post_dia    = 5;
post_hole   = 2.5;       // M2.5 screw
post_h      = 5;         // standoff height

/* [Lid] */
lid_h       = 2.5;       // lid thickness
lip         = 1.2;       // inner lip for snap fit
lip_h       = 2;

// ── Helper: rounded box ─────────────────────────────────
module rounded_box(w, d, h, r) {
    hull() {
        for (x = [-w/2+r, w/2-r])
            for (y = [-d/2+r, d/2-r])
                translate([x, y, 0])
                    cylinder(h=h, r=r, $fn=40);
    }
}

// ── MAIN BOX (bottom part) ──────────────────────────────
module main_box() {
    difference() {
        // Outer shell
        rounded_box(outer_w, outer_d, outer_h - lid_h, corner_r);
        
        // Inner cavity
        translate([0, 0, wall])
            rounded_box(inner_w, inner_d, outer_h, corner_r - wall/2);
        
        // ── FRONT FACE CUTOUTS (Y = -outer_d/2) ────────
        
        // LCD viewing window
        translate([lcd_x, -outer_d/2 - 1, outer_h/2 + lcd_z])
            rotate([-90, 0, 0])
                rounded_lcd_cutout(lcd_view_w, lcd_view_h, wall + 2);
        
        // LCD PCB slot (recessed from inside)
        translate([lcd_x, -outer_d/2 + wall - 1, outer_h/2 + lcd_z])
            rotate([-90, 0, 0])
                rounded_lcd_cutout(lcd_pcb_w + 1, lcd_pcb_h + 1, 2);
        
        // Green LED hole (left of pair)
        translate([led_x_off - led_spacing/2, -outer_d/2 - 1, outer_h/2 + led_z_off])
            rotate([-90, 0, 0])
                cylinder(d=led_dia, h=wall+2, $fn=30);
        
        // Red LED hole (right of pair)
        translate([led_x_off + led_spacing/2, -outer_d/2 - 1, outer_h/2 + led_z_off])
            rotate([-90, 0, 0])
                cylinder(d=led_dia, h=wall+2, $fn=30);
        
        // LED labels (small indents)
        // G label indent
        translate([led_x_off - led_spacing/2, -outer_d/2 - 0.1, outer_h/2 + led_z_off + 5])
            rotate([-90, 0, 0])
                cylinder(d=2, h=0.8, $fn=20);
        // R label indent
        translate([led_x_off + led_spacing/2, -outer_d/2 - 0.1, outer_h/2 + led_z_off + 5])
            rotate([-90, 0, 0])
                cylinder(d=2, h=0.8, $fn=20);
        
        // IR LED hole (front face, far left)
        translate([ir_x, -outer_d/2 - 1, outer_h/2 + ir_z])
            rotate([-90, 0, 0])
                cylinder(d=ir_dia, h=wall+2, $fn=30);
        
        // IR window cone (wider outside for spread)
        translate([ir_x, -outer_d/2 - 0.1, outer_h/2 + ir_z])
            rotate([-90, 0, 0])
                cylinder(d1=ir_dia, d2=ir_dia+3, h=wall+0.2, $fn=30);
        
        // ── FRONT FACE — Button holes (3 buttons in a row, bottom-right)
        for (i = [0, 1, 2]) {
            translate([btn_x_start + i * btn_spacing, -outer_d/2 - 1, outer_h/2 + btn_z_pos])
                rotate([-90, 0, 0])
                    cylinder(d=btn_dia, h=wall+2, $fn=30);
        }
        
        // Button labels engraved on front face
        // "-" button
        translate([btn_x_start, -outer_d/2 - 0.1, outer_h/2 + btn_z_pos + 5])
            rotate([90, 0, 0])
                linear_extrude(0.8)
                    text("-", size=4, halign="center", valign="center", font="Liberation Sans:style=Bold");
        // "OK" button  
        translate([btn_x_start + btn_spacing, -outer_d/2 - 0.1, outer_h/2 + btn_z_pos + 5])
            rotate([90, 0, 0])
                linear_extrude(0.8)
                    text("OK", size=3, halign="center", valign="center", font="Liberation Sans:style=Bold");
        // "+" button
        translate([btn_x_start + 2*btn_spacing, -outer_d/2 - 0.1, outer_h/2 + btn_z_pos + 5])
            rotate([90, 0, 0])
                linear_extrude(0.8)
                    text("+", size=4, halign="center", valign="center", font="Liberation Sans:style=Bold");
        
        // Buzzer holes (circular pattern on top)
        translate([buzz_x, buzz_y, outer_h - lid_h - 1])
            for (i = [0:buzz_count-1]) {
                angle = i * 360 / buzz_count;
                translate([buzz_r * cos(angle), buzz_r * sin(angle), 0])
                    cylinder(d=buzz_dia, h=wall+lid_h+2, $fn=20);
            }
        // Center buzzer hole
        translate([buzz_x, buzz_y, outer_h - lid_h - 1])
            cylinder(d=buzz_dia, h=wall+lid_h+2, $fn=20);
        
        // ── RIGHT SIDE — DHT22 vent slots ───────────────
        for (i = [0:dht_slots-1]) {
            translate([outer_w/2 - 1, 
                       -((dht_slots-1) * dht_slot_sp)/2 + i * dht_slot_sp, 
                       outer_h/2 + dht_z_off])
                rotate([0, 90, 0])
                    rounded_slot(dht_slot_w, dht_slot_h, wall+2);
        }
        
        // ── BACK FACE — USB port ────────────────────────
        translate([0, outer_d/2 - 1, outer_h/2 + usb_z])
            rotate([90, 0, 0])
                rounded_slot(usb_w, usb_h, wall+2);
        
        // ── BOTTOM — ventilation slots ──────────────────
        for (i = [-2:2]) {
            translate([i * 12, 0, -1])
                rounded_slot(3, 30, wall+2);
        }
    }
    

    
    // ── Screw posts at top corners for lid ──────────────
    // Solid posts that go from bottom to top of box, with screw holes
    screw_inset = 5;
    for (x = [-inner_w/2 + screw_inset, inner_w/2 - screw_inset])
        for (y = [-inner_d/2 + screw_inset, inner_d/2 - screw_inset])
            translate([x, y, wall])
                difference() {
                    cylinder(d=6, h=outer_h - lid_h - wall, $fn=30);
                    translate([0, 0, outer_h - lid_h - wall - 8])
                        cylinder(d=post_hole, h=10, $fn=20);
                }
}

// ── LCD mount rails ─────────────────────────────────────
module lcd_mount_rails() {
    rail_w = 2;
    rail_h = 2;
    for (x = [-lcd_pcb_w/2 - 0.5, lcd_pcb_w/2 + 0.5 - rail_w])
        translate([x, 0, -lcd_pcb_h/2])
            cube([rail_w, 4, lcd_pcb_h]);
}

// ── Rounded slot helper ─────────────────────────────────
module rounded_slot(w, h, depth) {
    hull() {
        for (y = [-h/2 + w/2, h/2 - w/2])
            translate([0, y, 0])
                cylinder(d=w, h=depth, $fn=20);
    }
}

// ── Rounded LCD cutout ──────────────────────────────────
module rounded_lcd_cutout(w, h, depth) {
    r = 1.5;
    hull() {
        for (x = [-w/2+r, w/2-r])
            for (y = [-h/2+r, h/2-r])
                translate([x, y, 0])
                    cylinder(r=r, h=depth, $fn=20);
    }
}

// ── LID ─────────────────────────────────────────────────
module lid() {
    difference() {
        // Simple flat lid plate
        rounded_box(outer_w, outer_d, lid_h, corner_r);
        
        // Screw holes matching the corner posts
        screw_inset = 5;
        for (x = [-inner_w/2 + screw_inset, inner_w/2 - screw_inset])
            for (y = [-inner_d/2 + screw_inset, inner_d/2 - screw_inset])
                translate([x, y, -1])
                    cylinder(d=post_hole + 0.4, h=lid_h + 2, $fn=20);
        
        // Countersink for screw heads
        for (x = [-inner_w/2 + screw_inset, inner_w/2 - screw_inset])
            for (y = [-inner_d/2 + screw_inset, inner_d/2 - screw_inset])
                translate([x, y, lid_h - 1.2])
                    cylinder(d1=post_hole + 0.4, d2=post_hole + 4, h=1.3, $fn=20);
    }
    
    // "SmartThermo" label on top
    translate([0, 8, lid_h - 0.4])
        linear_extrude(0.5)
            text("SmartThermo", size=6, halign="center", valign="center",
                 font="Liberation Sans:style=Bold");
    
    // Version label
    translate([0, 0, lid_h - 0.4])
        linear_extrude(0.5)
            text("v2.0", size=3.5, halign="center", valign="center",
                 font="Liberation Sans");
}

// ── BUTTON CAPS (print separately) ──────────────────────
module button_cap() {
    // Press-fit cap for tactile switch
    cap_d   = btn_dia - 0.3;
    cap_h   = 4;
    stem_d  = 3;
    stem_h  = 3;
    
    // Cap top
    cylinder(d=cap_d + 2, h=1.5, $fn=30);
    // Stem
    translate([0, 0, -stem_h])
        cylinder(d=stem_d, h=stem_h + 0.1, $fn=30);
}

// ── RENDER ──────────────────────────────────────────────

// Main box
main_box();

// Lid — placed to the side for printing
translate([outer_w + 15, 0, lid_h])
    rotate([180, 0, 0])
        lid();

// Button caps — placed to the side
for (i = [0:2])
    translate([-outer_w/2 - 15, -15 + i*12, 0])
        button_cap();
