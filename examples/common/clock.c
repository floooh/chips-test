#include "sokol_time.h"

static int frequency;
static int overrun_ticks;
static uint32_t frame_count;
static uint64_t last_time_stamp;
static int ticks_to_run;

void clock_init(uint32_t freq) {
    frequency = freq;
    frame_count = 0;
    ticks_to_run = 0;
    overrun_ticks = 0;
    last_time_stamp = stm_now();
}
int32_t clock_ticks_to_run(void) {
    frame_count++;
    double frame_time_ms = stm_ms(stm_laptime(&last_time_stamp));
    if (frame_time_ms < 24.0) {
        frame_time_ms = 16.666667;
    }
    else {
        frame_time_ms = 33.333334;
    }
    ticks_to_run = (int) (((frequency * frame_time_ms) / 1000.0) - overrun_ticks);
    if (ticks_to_run < 1) {
        ticks_to_run = 1;
    }
    return ticks_to_run;
}
void clock_ticks_executed(int ticks_executed) {
    if (ticks_executed > ticks_to_run) {
        overrun_ticks = ticks_executed - ticks_to_run;
    }
    else {
        overrun_ticks = 0;
    }
}
uint32_t clock_frame_count(void) {
    return frame_count;
}
