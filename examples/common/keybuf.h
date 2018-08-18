#pragma once
/*
    Simple playback-buffer to feed keyboard input into emulators.
*/

/* put a text for playback into keybuf, frame_delay is number of frames between keys */
extern void keybuf_put(int start_delay, int key_delay, const char* text);
/* get next key to feed into emulator, returns 0 if no key to feed */
extern uint8_t keybuf_get();
