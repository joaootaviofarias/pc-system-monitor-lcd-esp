#include <lvgl.h>
#include <stdio.h>
#include "monitor_ui.h"

LV_FONT_DECLARE(lv_font_orbitron_bold_36);
LV_FONT_DECLARE(lv_font_orbitron_medium_12);
LV_FONT_DECLARE(lv_font_orbitron_bold_12);
LV_FONT_DECLARE(lv_font_phosphor_24);

#define C_CPU    lv_color_make(  0, 136, 255)
#define C_GPU    lv_color_make(0, 255,   208)
#define C_RAM    lv_color_make(255, 0,   60)
#define C_BG     lv_color_make(  5,  10,  14)
#define C_TRACK  lv_color_make( 28,  41,  46)
#define C_WHITE  lv_color_make(255, 255, 255)

#define FA_CPU    "\ue610" // Example hex for microchip icon
#define FA_GPU    "\ue612" // Example hex for desktop/gpu icon
#define FA_RAM    "\ue9c4" // Example hex for memory/bookmark icon

// Single Arc Configuration
#define W_ARC    10          // Thicker arc 
#define SZ_ARC   220         // Universal size for the segmented arc

#define TEMP_WARN_C   60  
#define TEMP_DANGER_C 85  

static lv_obj_t *scr_monitor = NULL;

static lv_obj_t *arc_cpu, *arc_gpu, *arc_ram;
static lv_obj_t *lbl_temp_big;
static lv_obj_t *lbl_cpu_val, *lbl_gpu_val, *lbl_ram_val;

static lv_obj_t *chart;
static lv_chart_series_t *ser_cpu;
static lv_chart_series_t *ser_gpu;
static lv_chart_series_t *ser_ram;
static lv_obj_t *chart_frame;

static lv_color_t temp_to_color(uint8_t t) {
    if (t < TEMP_WARN_C) return lv_color_make(0, 229, 255);
    else if (t < TEMP_DANGER_C) return lv_color_make(255, 215, 0);
    else return lv_color_make(255, 0,   60);
}

// Added start_angle and end_angle to define distinct sections
static lv_obj_t *create_arc(lv_obj_t *parent, lv_coord_t size, lv_color_t color, uint8_t init_val, uint16_t start_angle, uint16_t end_angle) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_center(arc);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_arc_set_bg_angles(arc, start_angle, end_angle);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, init_val);

    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(arc, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_arc_color(arc, color, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 2, LV_PART_MAIN);
    //lv_obj_set_style_arc_opa(arc, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);

    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, W_ARC, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);

    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    return arc;
}

static lv_obj_t *create_tag(lv_obj_t *parent, const char *text, lv_color_t color) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_orbitron_medium_12, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_letter_space(lbl, 1, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    return lbl;
}

static lv_obj_t *create_fa_icon(lv_obj_t *parent, const char *unicode_str, lv_color_t color) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, unicode_str);
    lv_obj_set_style_text_font(lbl, &lv_font_phosphor_24, 0); // Use your custom font
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

static lv_obj_t *create_val_label(lv_obj_t *parent, const char *text, lv_color_t color, const lv_font_t *font) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    return lbl;
}

static void set_arc_anim_cb(void * var, int32_t v) {
    lv_arc_set_value((lv_obj_t *)var, v);
}

static void animate_arc_value(lv_obj_t * arc, int32_t new_value) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, set_arc_anim_cb);
    lv_anim_set_time(&a, 400); 
    lv_anim_set_values(&a, lv_arc_get_value(arc), new_value);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out); 
    lv_anim_start(&a);
}

void monitor_ui_init(void) {
    scr_monitor = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_monitor, C_BG, 0);

    // Split 270 degrees into 3 equal 85-degree sections with 5-degree gaps
    // LVGL angles: 0=Right, 90=Bottom, 180=Left, 270=Top
    arc_cpu = create_arc(scr_monitor, SZ_ARC, C_CPU, 0, 140, 220);
    arc_gpu = create_arc(scr_monitor, SZ_ARC, C_GPU, 0, 230, 310);
    arc_ram = create_arc(scr_monitor, SZ_ARC, C_RAM, 0, 320, 40);

    lv_obj_t *lbl_cpu_tag = create_fa_icon(scr_monitor, FA_CPU, C_CPU);
    lv_obj_t *lbl_gpu_tag = create_fa_icon(scr_monitor, FA_GPU, C_GPU);
    lv_obj_t *lbl_ram_tag = create_fa_icon(scr_monitor, FA_RAM, C_RAM);   
    
    lbl_cpu_val = create_val_label(scr_monitor, "0%", C_CPU, &lv_font_orbitron_bold_12);
    lbl_gpu_val = create_val_label(scr_monitor, "0%", C_GPU, &lv_font_orbitron_bold_12);
    lbl_ram_val = create_val_label(scr_monitor, "0%", C_RAM, &lv_font_orbitron_bold_12);

    // CPU (135 deg): x = 80 * cos(135) = -56, y = 80 * sin(135) = 56
    lv_obj_align(lbl_cpu_tag, LV_ALIGN_CENTER, -82,  -8);
    lv_obj_align(lbl_cpu_val, LV_ALIGN_CENTER, -82,   9);

    // GPU (225 deg): x = 80 * cos(225) = -56, y = 80 * sin(225) = -56
    lv_obj_align(lbl_gpu_tag, LV_ALIGN_CENTER,   0, -82);
    lv_obj_align(lbl_gpu_val, LV_ALIGN_CENTER,   0, -66);

    // RAM (315 deg): x = 80 * cos(315) = 56, y = 80 * sin(315) = -56
    lv_obj_align(lbl_ram_tag, LV_ALIGN_CENTER,  80,  -8);
    lv_obj_align(lbl_ram_val, LV_ALIGN_CENTER,  80,   8);

    // Position temperature in the bottom opening gap (45 to 135 degrees)
    lbl_temp_big = create_val_label(scr_monitor, "--\xC2\xB0\x43", temp_to_color(20), &lv_font_orbitron_bold_36);
    lv_obj_align(lbl_temp_big, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *lbl_sub = create_tag(scr_monitor, "CPU TEMP", C_WHITE);
    lv_obj_align(lbl_sub, LV_ALIGN_CENTER, 0, 15);

      // =========================
    // CHART
    // =========================

    chart = lv_chart_create(scr_monitor);

    lv_obj_set_size(chart, 120, 50);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 70);

    lv_obj_set_style_bg_color(chart, C_CPU, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(chart, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_radius(chart, 0, LV_PART_MAIN);

    // --- THE FIX: Remove the manual border completely! ---
    // LVGL's tick system draws the vertical line automatically.
    lv_obj_set_style_border_width(chart, 0, LV_PART_MAIN); 

    // Add exactly 2px padding on all sides so the 2px tick lines 
    // don't get clipped by the widget's bounding box.
    lv_obj_set_style_pad_top(chart, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(chart, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_left(chart, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(chart, 10, LV_PART_MAIN);

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

    lv_chart_set_point_count(chart, 40);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    // Hide divisions/grid
    lv_chart_set_div_line_count(chart, 0, 0);

    // Ticks (Length of -8px pointing inward). This ALSO draws the vertical spine!
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, -6, 0, 3, 1, false, 10);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_SECONDARY_Y, -6, 0, 3, 1, false, 10);

    // Style the markers AND the vertical lines together
    lv_obj_set_style_line_color(chart, lv_color_white(), LV_PART_TICKS);
    lv_obj_set_style_line_width(chart, 2, LV_PART_TICKS);

    // Smooth lines (Data series lines)
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);

    // Remove points
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);

    // Series
    ser_cpu = lv_chart_add_series(chart, C_CPU, LV_CHART_AXIS_PRIMARY_Y);
    ser_gpu = lv_chart_add_series(chart, C_GPU, LV_CHART_AXIS_PRIMARY_Y);
    ser_ram = lv_chart_add_series(chart, C_RAM, LV_CHART_AXIS_PRIMARY_Y);

    // Initialize with 0
    for(int i = 0; i < 40; i++) {
        lv_chart_set_next_value(chart, ser_cpu, 0);
        lv_chart_set_next_value(chart, ser_gpu, 0);
        lv_chart_set_next_value(chart, ser_ram, 0);
    }
}

void monitor_ui_show(void) {
    if (lv_scr_act() != scr_monitor) {
        lv_scr_load(scr_monitor);
    }
}

void monitor_ui_update(const monitor_data_t *data) {
    char buf[12];
    lv_color_t tc = temp_to_color(data->cpu_temp_c);

    animate_arc_value(arc_cpu, data->cpu_pct);
    animate_arc_value(arc_gpu, data->gpu_pct);
    animate_arc_value(arc_ram, data->ram_pct);

    snprintf(buf, sizeof(buf), "%d\xC2\xB0\x43", data->cpu_temp_c);
    lv_label_set_text(lbl_temp_big, buf);
    lv_obj_set_style_text_color(lbl_temp_big, tc, 0);
    lv_obj_align(lbl_temp_big, LV_ALIGN_CENTER, 0, -10); 

    snprintf(buf, sizeof(buf), "%d%%", data->cpu_pct);
    lv_label_set_text(lbl_cpu_val, buf);
    lv_obj_align(lbl_cpu_val, LV_ALIGN_CENTER, -82, 9);

    snprintf(buf, sizeof(buf), "%d%%", data->gpu_pct);
    lv_label_set_text(lbl_gpu_val, buf);
    lv_obj_align(lbl_gpu_val, LV_ALIGN_CENTER, 0, -66);

    snprintf(buf, sizeof(buf), "%d%%", data->ram_pct);
    lv_label_set_text(lbl_ram_val, buf);
    lv_obj_align(lbl_ram_val, LV_ALIGN_CENTER, 80, 8);

        // =========================
    // UPDATE CHART
    // =========================

    lv_chart_set_next_value(chart, ser_cpu, data->cpu_pct);
    lv_chart_set_next_value(chart, ser_gpu, data->gpu_pct);
    lv_chart_set_next_value(chart, ser_ram, data->ram_pct);

    lv_chart_refresh(chart);
}