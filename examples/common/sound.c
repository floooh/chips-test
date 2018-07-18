#include "sokol_audio.h"

#define SAMPLE_BUFFER_SIZE (32)
static int sample_pos;
static float sample_buffer[SAMPLE_BUFFER_SIZE];

void sound_init(void) {
    saudio_setup(&(saudio_desc){0});
}
void sound_shutdown(void) {
    saudio_shutdown();
}
int sound_sample_rate(void) {
    return saudio_sample_rate();
}
void sound_push(float sample) {
    sample_buffer[sample_pos++] = sample;
    if (sample_pos == SAMPLE_BUFFER_SIZE) {
        sample_pos = 0;
        saudio_push(sample_buffer, SAMPLE_BUFFER_SIZE);
    }
}
