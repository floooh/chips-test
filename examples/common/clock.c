#include "sokol_time.h"

static uint32_t frame_count;
static uint64_t last_time_stamp;

void clock_init(void) {
    frame_count = 0;
    last_time_stamp = stm_now();
}

double clock_frame_time(void) {
    frame_count++;
    double frame_time_sec = stm_sec(stm_laptime(&last_time_stamp));
    if (frame_time_sec < 0.024) {
        frame_time_sec = 0.016666667;
    }
    else {
        frame_time_sec = 0.033333334;
    }
    return frame_time_sec;
}

uint32_t clock_frame_count(void) {
    return frame_count;
}
