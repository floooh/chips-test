#pragma once
/*
    Emulator frame timing helper functions.
*/
void clock_init(void);
uint32_t clock_frame_time(void);
uint32_t clock_frame_count_60hz(void);

/*== IMPLEMENTATION ==========================================================*/
#ifdef COMMON_IMPL
#include "sokol_app.h"
#include <assert.h>

typedef struct {
    bool valid;
    uint64_t cur_time;
} clock_state_t;
static clock_state_t clck;

void clock_init(void) {
    clck = (clock_state_t) {
        .valid = true,
        .cur_time = 0,
    };
}

uint32_t clock_frame_time(void) {
    assert(clck.valid);
    uint32_t frame_time_us = (uint32_t) (sapp_frame_duration() * 1000000.0);
    // prevent death-spiral on host systems that are too slow to emulate
    // in real time, or during long frames (e.g. debugging)
    if (frame_time_us > 24000) {
        frame_time_us = 24000;
    }
    clck.cur_time += frame_time_us;
    return frame_time_us;
}

uint32_t clock_frame_count_60hz(void) {
    assert(clck.valid);
    return (uint32_t) (clck.cur_time / 16667);
}
#endif /* COMMON_IMPL */
