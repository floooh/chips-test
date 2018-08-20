#include <stdint.h>
#include <string.h>
#include "keybuf.h"

#define KEYBUF_MAX_KEYS (1024)
static uint8_t buf[KEYBUF_MAX_KEYS];
static int cur_pos;
static int delay_count;
static int key_delay;

void keybuf_put(int start_delay, int key_delay_, const char* text) {
    if (!text) {
        return;
    }
    if (key_delay_ <= 0) {
        key_delay_ = 1;
    }
    if (start_delay <= 0) {
        start_delay = 1;
    }
    key_delay = key_delay_;
    delay_count = start_delay;
    int len = (int) strlen(text);
    if ((len+1) < KEYBUF_MAX_KEYS) {
        strcpy((char*)buf, text);
    }
    else {
        buf[0] = 0;
    }
    cur_pos = 0;
}

uint8_t keybuf_get() {
    if (delay_count == 0) {
        delay_count = key_delay;
        if ((cur_pos < KEYBUF_MAX_KEYS) && (buf[cur_pos] != 0)) {
            uint8_t c = buf[cur_pos++];
            return (c == '\n') ? 0x0D : c;
        }
    }
    else {
        delay_count--;
    }
    return 0;
}
