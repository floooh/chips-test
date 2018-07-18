#pragma once
/*
    Emulator clock helper functions.
*/
extern void clock_init(int freq);
extern int clock_ticks_to_run(void);
extern void clock_ticks_executed(int ticks);
extern uint32_t clock_frame_count(void);
