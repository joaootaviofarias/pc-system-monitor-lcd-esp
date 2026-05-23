#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>

typedef struct {
    uint8_t cpu_pct;
    uint8_t gpu_pct;
    uint8_t ram_pct;
    uint8_t cpu_temp_c;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} monitor_data_t;

void monitor_ui_init(void);

void ui_show_clock(int, int);
void ui_show_monitor(void);
void ui_show_wait(void);

void monitor_ui_update(const monitor_data_t *data);

#ifdef __cplusplus
}
#endif