#pragma once
/*
    Emulator frame timing helper functions.
*/
extern void clock_init(void);
extern uint32_t clock_frame_time(void);
extern uint32_t clock_frame_count(void);

/*== IMPLEMENTATION ==========================================================*/
#ifdef COMMON_IMPL
#include "sokol_time.h"

typedef struct {
    uint32_t frame_count;
    uint64_t last_time_stamp;
} clock_state;
static clock_state clck;

void clock_init(void) {
    stm_setup();
    clck.frame_count = 0;
    clck.last_time_stamp = stm_now();
}

uint32_t clock_frame_time(void) {
    clck.frame_count++;
    uint32_t frame_time_us = (uint32_t) stm_us(stm_round_to_common_refresh_rate(stm_laptime(&clck.last_time_stamp)));
    // prevent death-spiral on host systems that are too slow to emulate
    // in real time, or during long frames (e.g. debugging)
    if (frame_time_us > 24000) {
        frame_time_us = 24000;
    }
    return frame_time_us;
}

uint32_t clock_frame_count(void) {
    return clck.frame_count;
}
#endif /* COMMON_IMPL */
