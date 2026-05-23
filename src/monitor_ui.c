#include "monitor_ui.h"
#include <stdio.h>

LV_FONT_DECLARE(lv_font_montserrat_10)
LV_FONT_DECLARE(lv_font_montserrat_12)
LV_FONT_DECLARE(lv_font_montserrat_28)
LV_FONT_DECLARE(lv_font_montserrat_48)


#define C_CPU    lv_color_make(  0, 229, 255)  /* cyan        */
#define C_GPU    lv_color_make(118, 255,   3)  /* lime        */
#define C_RAM    lv_color_make(255, 145,   0)  /* orange      */
#define C_BG     lv_color_make(  5,  10,  14)  /* near-black  */
#define C_TRACK  lv_color_make( 12,  28,  38)  /* dim track   */
#define C_WHITE  lv_color_make(255, 255, 255)

#define ARC_BG_START   135   
#define ARC_BG_END      45   

#define W_ARC    8           
#define RING_GAP 6           
#define RING_STEP (W_ARC + RING_GAP)   

#define SZ_CPU   220                       
#define SZ_GPU   (SZ_CPU  - 2*RING_STEP)  
#define SZ_RAM   (SZ_GPU  - 2*RING_STEP)  
#define SZ_TEMP  (SZ_RAM  - 2*RING_STEP)  

#define TEMP_MIN_C   20
#define TEMP_MAX_C   99
#define TEMP_WARN_C   60  
#define TEMP_DANGER_C 85  

static lv_obj_t *scr_monitor = NULL;
static lv_obj_t *scr_clock   = NULL;
static lv_obj_t *scr_wait    = NULL;

static lv_obj_t *arc_cpu, *arc_gpu, *arc_ram, *arc_temp;
static lv_obj_t *lbl_temp_big;
static lv_obj_t *lbl_cpu_tag, *lbl_gpu_tag, *lbl_ram_tag;
static lv_obj_t *lbl_cpu_val, *lbl_gpu_val, *lbl_ram_val;

static lv_obj_t *lbl_clock_time;

static lv_color_t temp_to_color(uint8_t t)
{
    if (t < TEMP_WARN_C) {
        return lv_color_make(0, 229, 255);
    } else if (t < TEMP_DANGER_C) {
        return lv_color_make(255, 255, 0);
    } else {
        return lv_color_make(255, 23, 68);
    }
}

static int temp_to_pct(uint8_t t)
{
    int pct = (int)((t - TEMP_MIN_C) * 100 / (TEMP_MAX_C - TEMP_MIN_C));
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

static lv_obj_t *create_arc(lv_obj_t  *parent, lv_coord_t size, lv_color_t color, uint8_t init_val)
{
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_center(arc);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_arc_set_bg_angles(arc, ARC_BG_START, ARC_BG_END);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, init_val);

    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(arc, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_arc_color(arc,  C_TRACK,      LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc,  W_ARC,        LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc,    LV_OPA_40,    LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true,        LV_PART_MAIN);

    lv_obj_set_style_arc_color(arc,  color,        LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc,  W_ARC,        LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true,         LV_PART_INDICATOR);

    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    return arc;
}

static lv_obj_t *create_tag(lv_obj_t *parent, const char *text, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl,  &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl, color,       0);
    lv_obj_set_style_text_opa(lbl,   LV_OPA_COVER,   0);
    lv_obj_set_style_text_decor(lbl, LV_TEXT_DECOR_NONE, 0); 
    lv_obj_set_style_text_letter_space(lbl, 1, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    return lbl;
}

static lv_obj_t *create_val_label(lv_obj_t *parent, const char *text, lv_color_t color, const lv_font_t *font)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl,  font,  0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    return lbl;
}

static void set_arc_anim_cb(void * var, int32_t v)
{
    lv_arc_set_value((lv_obj_t *)var, v);
}

static void animate_arc_value(lv_obj_t * arc, int32_t new_value)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, set_arc_anim_cb);
    lv_anim_set_time(&a, 400); 
    lv_anim_set_values(&a, lv_arc_get_value(arc), new_value);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out); 
    lv_anim_start(&a);
}

void monitor_ui_init(void)
{
    scr_monitor = lv_obj_create(NULL);
    scr_clock   = lv_obj_create(NULL);
    scr_wait    = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(scr_monitor, C_BG, 0);
    lv_obj_set_style_bg_color(scr_clock,   C_BG, 0);
    lv_obj_set_style_bg_color(scr_wait,    C_BG, 0);

    lv_obj_t *spinner = lv_spinner_create(scr_wait, 1000, 60);
    lv_obj_set_size(spinner, 60, 60);
    lv_obj_center(spinner);
    lv_obj_set_style_arc_color(spinner, C_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, C_CPU,   LV_PART_INDICATOR);

    lv_obj_t *lbl_wait = lv_label_create(scr_wait);
    lv_label_set_text(lbl_wait, "AWAITING DATA LINK...");
    lv_obj_set_style_text_font(lbl_wait, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_wait, C_WHITE, 0);
    lv_obj_align(lbl_wait, LV_ALIGN_CENTER, 0, 45);

    lbl_clock_time = lv_label_create(scr_clock);
    lv_label_set_text(lbl_clock_time, "00:00");
    lv_obj_set_style_text_font(lbl_clock_time, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_clock_time, C_WHITE, 0);
    lv_obj_center(lbl_clock_time);


    arc_cpu  = create_arc(scr_monitor, SZ_CPU,  C_CPU,               0);
    arc_gpu  = create_arc(scr_monitor, SZ_GPU,  C_GPU,               0);
    arc_ram  = create_arc(scr_monitor, SZ_RAM,  C_RAM,               0);
    arc_temp = create_arc(scr_monitor, SZ_TEMP, temp_to_color(20), temp_to_pct(20));

    lbl_temp_big = create_val_label(scr_monitor, "--\xC2\xB0\x43", temp_to_color(20), &lv_font_montserrat_28);
    lv_obj_align(lbl_temp_big, LV_ALIGN_CENTER, 0, -6);

    lbl_cpu_tag = create_tag(scr_monitor, "CPU", C_CPU);
    lbl_gpu_tag = create_tag(scr_monitor, "GPU", C_GPU);
    lbl_ram_tag = create_tag(scr_monitor, "RAM", C_RAM);
    lv_obj_align(lbl_cpu_tag, LV_ALIGN_CENTER, -38, 76);
    lv_obj_align(lbl_gpu_tag, LV_ALIGN_CENTER,   0, 76);
    lv_obj_align(lbl_ram_tag, LV_ALIGN_CENTER,  38, 76);

    lbl_cpu_val = create_val_label(scr_monitor, "0%", C_CPU, &lv_font_montserrat_12);
    lbl_gpu_val = create_val_label(scr_monitor, "0%", C_GPU, &lv_font_montserrat_12);
    lbl_ram_val = create_val_label(scr_monitor, "0%", C_RAM, &lv_font_montserrat_12);
    lv_obj_align(lbl_cpu_val, LV_ALIGN_CENTER, -38, 90);
    lv_obj_align(lbl_gpu_val, LV_ALIGN_CENTER,   0, 90);
    lv_obj_align(lbl_ram_val, LV_ALIGN_CENTER,  38, 90);

    lv_obj_t *lbl_sub = create_tag(scr_monitor, "CPU TEMP", C_WHITE);
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_sub, LV_ALIGN_CENTER, 0, 12);

    lv_scr_load(scr_wait);
}


void ui_show_wait(void) 
{
    if (lv_scr_act() != scr_wait) {
        lv_scr_load(scr_wait);
    }
}

void ui_show_monitor(void) 
{
    if (lv_scr_act() != scr_monitor) {
        lv_scr_load(scr_monitor);
    }
}

void ui_show_clock(int hours, int minutes) 
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", hours, minutes);
    lv_label_set_text(lbl_clock_time, buf);

    if (lv_scr_act() != scr_clock) {
        lv_scr_load(scr_clock);
    }
}

void monitor_ui_update(const monitor_data_t *data)
{
    char buf[12];
    lv_color_t tc = temp_to_color(data->cpu_temp_c);

    animate_arc_value(arc_cpu, data->cpu_pct);
    animate_arc_value(arc_gpu, data->gpu_pct);
    animate_arc_value(arc_ram, data->ram_pct);
    animate_arc_value(arc_temp, temp_to_pct(data->cpu_temp_c));

    lv_obj_set_style_arc_color(arc_temp, tc, LV_PART_INDICATOR);

    snprintf(buf, sizeof(buf), "%d\xC2\xB0\x43", data->cpu_temp_c);
    lv_label_set_text(lbl_temp_big, buf);
    lv_obj_set_style_text_color(lbl_temp_big, tc, 0);
    lv_obj_align(lbl_temp_big, LV_ALIGN_CENTER, 0, -6); 

    snprintf(buf, sizeof(buf), "%d%%", data->cpu_pct);
    lv_label_set_text(lbl_cpu_val, buf);
    lv_obj_align(lbl_cpu_val, LV_ALIGN_CENTER, -38, 90);

    snprintf(buf, sizeof(buf), "%d%%", data->gpu_pct);
    lv_label_set_text(lbl_gpu_val, buf);
    lv_obj_align(lbl_gpu_val, LV_ALIGN_CENTER, 0, 90);

    snprintf(buf, sizeof(buf), "%d%%", data->ram_pct);
    lv_label_set_text(lbl_ram_val, buf);
    lv_obj_align(lbl_ram_val, LV_ALIGN_CENTER, 38, 90);
}