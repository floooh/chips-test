#pragma once
/*
    Emulator frame timing helper functions.
*/
extern void clock_init(void);
extern double clock_frame_time(void);
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

double clock_frame_time(void) {
    clck.frame_count++;
    double frame_time_sec = stm_sec(stm_laptime(&clck.last_time_stamp));
    if (frame_time_sec < 0.024) {
        frame_time_sec = 0.016666667;
    }
    else {
        frame_time_sec = 0.033333333;
    }
    return frame_time_sec;
}

uint32_t clock_frame_count(void) {
    return clck.frame_count;
}
#endif /* COMMON_IMPL */
