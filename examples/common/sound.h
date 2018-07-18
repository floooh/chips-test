#pragma once
/*
    Sound output helper functions.
*/

extern void sound_init(void);
extern void sound_shutdown(void);
extern int sound_sample_rate(void);
extern void sound_push(float sample);
