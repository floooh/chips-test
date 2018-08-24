#pragma once
/*
    Simple playback-buffer to feed keyboard input into emulators.
*/

/* put a text for playback into keybuf, frame_delay is number of frames between keys */
extern void keybuf_put(int start_delay, int key_delay, const char* text);
/* get next key to feed into emulator, returns 0 if no key to feed */
extern uint8_t keybuf_get();

/*== IMPLEMENTATION ==========================================================*/
#ifdef COMMON_IMPL
#include <stdint.h>
#include <string.h>
#include "keybuf.h"

#define KEYBUF_MAX_KEYS (1024)
typedef struct {
    int cur_pos;
    int delay_count;
    int key_delay;
    uint8_t buf[KEYBUF_MAX_KEYS];
} keybuf_state;
static keybuf_state keybuf;

void keybuf_put(int start_delay, int key_delay, const char* text) {
    if (!text) {
        return;
    }
    if (key_delay <= 0) {
        key_delay = 1;
    }
    if (start_delay <= 0) {
        start_delay = 1;
    }
    keybuf.key_delay = key_delay;
    keybuf.delay_count = start_delay;
    int len = (int) strlen(text);
    if ((len+1) < KEYBUF_MAX_KEYS) {
        strcpy((char*)keybuf.buf, text);
    }
    else {
        keybuf.buf[0] = 0;
    }
    keybuf.cur_pos = 0;
}

uint8_t keybuf_get() {
    if (keybuf.delay_count == 0) {
        keybuf.delay_count = keybuf.key_delay;
        if ((keybuf.cur_pos < KEYBUF_MAX_KEYS) && (keybuf.buf[keybuf.cur_pos] != 0)) {
            uint8_t c = keybuf.buf[keybuf.cur_pos++];
            return (c == '\n') ? 0x0D : c;
        }
    }
    else {
        keybuf.delay_count--;
    }
    return 0;
}
#endif /* COMMON_IMPL */
