#include <lvgl.h>
#include "wait_ui.h"

LV_FONT_DECLARE(lv_font_orbitron_medium_12);

#define C_BG     lv_color_make(5, 10, 14)
#define C_WHITE  lv_color_make(255, 255, 255)

static lv_obj_t *scr_wait = NULL;
static lv_obj_t *lbl_cursor = NULL;

// Timer callback to blink the cursor on and off
static void blink_timer_cb(lv_timer_t * timer) {
    static uint8_t hidden = 0;
    if (hidden) {
        lv_obj_set_style_bg_opa(lbl_cursor, 255, 0); // Show
    } else {
        lv_obj_set_style_bg_opa(lbl_cursor, 0, 0);   // Hide
    }
    hidden = !hidden;
}

void wait_ui_init(void) {
    scr_wait = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_wait, C_BG, 0);

    // Create an invisible flex container to hold text and cursor side-by-side
    // This prevents the text from shifting left/right when the cursor blinks
    lv_obj_t * cont = lv_obj_create(scr_wait);
    lv_obj_remove_style_all(cont); 
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_center(cont);

    // Main text label
    lv_obj_t *lbl_wait = lv_label_create(cont);
    lv_label_set_text(lbl_wait, "Waiting serial data");
    lv_obj_set_style_text_font(lbl_wait, &lv_font_orbitron_medium_12, 0);
    lv_obj_set_style_text_color(lbl_wait, C_WHITE, 0);

    // Blinking cursor label (vertical bar)
    lbl_cursor = lv_obj_create(cont);
    lv_obj_set_size(lbl_cursor, 8, 14); // Tweak width/height to match your font
    lv_obj_set_style_bg_color(lbl_cursor, C_WHITE, 0);
    lv_obj_set_style_radius(lbl_cursor, 0, 0); // Sharp corners
    lv_obj_set_style_border_width(lbl_cursor, 0, 0);

    // Start a timer to blink the cursor every 500ms
    lv_timer_create(blink_timer_cb, 500, NULL);
}

void wait_ui_show(void) {
    if (lv_scr_act() != scr_wait) {
        lv_scr_load(scr_wait);
    }
}