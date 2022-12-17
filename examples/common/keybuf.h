#pragma once
/*
    Simple playback-buffer to feed keyboard input into emulators.

    Special embedded commands:

    ${wait:20} - wait 20 frames before continuing
*/
#include <stdint.h>

typedef struct {
    int key_delay_frames;
} keybuf_desc_t;

// initialize the keybuf with a base-delay between keys in 60 Hz frames
void keybuf_init(const keybuf_desc_t* desc);
// put a text for playback into keybuf
void keybuf_put(const char* text);
// get next key to feed into emulator, call once per frame, returns 0 if no key to feed
uint8_t keybuf_get(uint32_t frame_time_us);
