#include <lvgl.h>
#include "wait_ui.h"

LV_FONT_DECLARE(lv_font_orbitron_medium_12);

#define C_BG     lv_color_make(5, 10, 14)
#define C_TRACK  lv_color_make(12, 28, 38)
#define C_CPU    lv_color_make(0, 229, 255)
#define C_WHITE  lv_color_make(255, 255, 255)

static lv_obj_t *scr_wait = NULL;

void wait_ui_init(void) {
    scr_wait = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_wait, C_BG, 0);

    lv_obj_t *spinner = lv_spinner_create(scr_wait, 1000, 40);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_center(spinner);
    lv_obj_set_style_arc_color(spinner, C_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, C_CPU, LV_PART_INDICATOR);

    lv_obj_t *lbl_wait = lv_label_create(scr_wait);
    lv_label_set_text(lbl_wait, "AWAITING DATA LINK...");
    lv_obj_set_style_text_font(lbl_wait, &lv_font_orbitron_medium_12, 0);
    lv_obj_set_style_text_color(lbl_wait, C_WHITE, 0);
    lv_obj_align(lbl_wait, LV_ALIGN_CENTER, 0, 45);
}

void wait_ui_show(void) {
    if (lv_scr_act() != scr_wait) {
        lv_scr_load(scr_wait);
    }
}