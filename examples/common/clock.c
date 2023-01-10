#include "sokol_app.h"
#include "clock.h"
#include <assert.h>

typedef struct {
    bool valid;
    uint64_t cur_time;
} clock_state_t;
static clock_state_t state;

void clock_init(void) {
    state = (clock_state_t) {
        .valid = true,
        .cur_time = 0,
    };
}

uint32_t clock_frame_time(void) {
    assert(state.valid);
    uint32_t frame_time_us = (uint32_t) (sapp_frame_duration() * 1000000.0);
    // prevent death-spiral on host systems that are too slow to emulate
    // in real time, or during long frames (e.g. debugging)
    if (frame_time_us > 24000) {
        frame_time_us = 24000;
    }
    state.cur_time += frame_time_us;
    return frame_time_us;
}

uint32_t clock_frame_count_60hz(void) {
    assert(state.valid);
    return (uint32_t) (state.cur_time / 16667);
}
