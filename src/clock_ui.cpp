#include <lvgl.h>
#include <stdio.h>
#include "clock_ui.h"

LV_FONT_DECLARE(lv_font_orbitron_bold_48);

#define C_BG     lv_color_make(5, 10, 14)
#define C_WHITE  lv_color_make(255, 255, 255)

static lv_obj_t *scr_clock = NULL;
static lv_obj_t *lbl_clock_time;
static lv_obj_t *arc_seconds;

void clock_ui_init(void) {
    scr_clock = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_clock, C_BG, 0);

    lbl_clock_time = lv_label_create(scr_clock);
    lv_label_set_text(lbl_clock_time, "00:00");
    lv_obj_set_style_text_font(lbl_clock_time, &lv_font_orbitron_bold_48, 0);
    lv_obj_set_style_text_color(lbl_clock_time, C_WHITE, 0);
    lv_obj_center(lbl_clock_time);

    /* --- The Sweeping Second Arc --- */
    arc_seconds = lv_arc_create(scr_clock);
    lv_obj_set_size(arc_seconds, 240, 240);
    lv_obj_center(arc_seconds);

    // Rotate 270 degrees so the "0" starting point is at the top (12 o'clock)
    lv_arc_set_rotation(arc_seconds, 270);
    lv_arc_set_bg_angles(arc_seconds, 0, 360);
    lv_arc_set_range(arc_seconds, 0, 60);

    // Disable the background track (make it invisible)
    lv_obj_set_style_arc_width(arc_seconds, 0, LV_PART_MAIN);

    // Style the sweeping indicator
    lv_obj_set_style_arc_color(arc_seconds, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_seconds, 10, LV_PART_INDICATOR);

    // Add neon glow
    lv_obj_set_style_shadow_color(arc_seconds, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_shadow_width(arc_seconds, 15, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_spread(arc_seconds, 1, LV_PART_INDICATOR);
     lv_obj_set_style_arc_rounded(arc_seconds, false, LV_PART_INDICATOR);

    lv_obj_remove_style(arc_seconds, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_seconds, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_anim_time(arc_seconds, 500, LV_PART_MAIN);
}

void clock_ui_show(void) {
    if (lv_scr_act() != scr_clock) {
        lv_scr_load(scr_clock);
    }
}

void clock_ui_update(int hr, int min, int sec) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", hr, min);
    lv_label_set_text(lbl_clock_time, buf);
    lv_arc_set_value(arc_seconds, sec);
}