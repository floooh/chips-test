#pragma once
/*
    Emulator frame timing helper functions.
*/
void clock_init(void);
uint32_t clock_frame_time(void);
uint32_t clock_frame_count_60hz(void);

/*== IMPLEMENTATION ==========================================================*/
#ifdef COMMON_IMPL
#include "sokol_time.h"
#include <assert.h>

typedef struct {
    bool valid;
    uint64_t cur_time;
    uint64_t last_time_stamp;
} clock_state_t;
static clock_state_t clck;

void clock_init(void) {
    stm_setup();
    clck = (clock_state_t) {
        .valid = true,
        .cur_time = 0,
        .last_time_stamp = stm_now(),
    };
}

uint32_t clock_frame_time(void) {
    assert(clck.valid);
    uint32_t frame_time_us = (uint32_t) stm_us(stm_round_to_common_refresh_rate(stm_laptime(&clck.last_time_stamp)));
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
