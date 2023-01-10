#pragma once
#include <stdint.h>

void clock_init(void);
uint32_t clock_frame_time(void);
uint32_t clock_frame_count_60hz(void);
