#pragma once
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