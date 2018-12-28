#pragma once
/*
    Simple playback-buffer to feed keyboard input into emulators.

    Special embedded commands:

    ${wait:20} - wait 20 frames before continuing
*/

/* initialize the keybuf with a base-delay between keys in frames */
extern void keybuf_init(int key_delay);
/* put a text for playback into keybuf, frame_delay is number of frames between keys */
extern void keybuf_put(const char* text);
/* get next key to feed into emulator, returns 0 if no key to feed */
extern uint8_t keybuf_get();

/*== IMPLEMENTATION ==========================================================*/
#ifdef COMMON_IMPL
#include <stdint.h>
#include <string.h>
#include "keybuf.h"

#define KEYBUF_MAX_KEYS (64 * 1024)
typedef struct {
    int cur_pos;
    int delay_count;
    int key_delay;
    uint8_t buf[KEYBUF_MAX_KEYS];
} keybuf_state;
static keybuf_state keybuf;

void keybuf_init(int key_delay) {
    memset(&keybuf, 0, sizeof(keybuf));
    keybuf.delay_count = key_delay;
    keybuf.key_delay = key_delay;
}

void keybuf_put(const char* text) {
    if (!text) {
        return;
    }
    keybuf.delay_count = 0;
    int len = (int) strlen(text);
    if ((len+1) < KEYBUF_MAX_KEYS) {
        strcpy((char*)keybuf.buf, text);
    }
    else {
        keybuf.buf[0] = 0;
    }
    keybuf.cur_pos = 0;
}

static uint8_t _keybuf_peek(void) {
    if (keybuf.cur_pos < KEYBUF_MAX_KEYS) {
        return keybuf.buf[keybuf.cur_pos];
    }
    else {
        return 0;
    }
}

static uint8_t _keybuf_next(void) {
    uint8_t c = _keybuf_peek();
    if (0 != c) {
        keybuf.cur_pos++;
    }
    return c;
}

static bool _keybuf_extract(uint8_t delim, uint8_t* buf, int buf_size) {
    for (int i = 0; i < buf_size; i++) {
        buf[i] = _keybuf_next();
        if (buf[i] == delim) {
            buf[i] = 0;
            return true;
        }
    }
    buf[buf_size-1] = 0;
    return false;
}

static uint8_t _keybuf_parse_cmd(void) {
    /* skip initial '{' */
    _keybuf_next();
    uint8_t key[8];
    uint8_t val[8];
    if (_keybuf_extract(':', key, sizeof(key))) {
        if (_keybuf_extract('}', val, sizeof(val))) {
            if (strcmp((const char*)key, "wait") == 0) {
                keybuf.delay_count = atoi((const char*)val);
                return 0;
            }
            else if (strcmp((const char*)key, "delay") == 0) {
                keybuf.key_delay = atoi((const char*)val);
                return 0;
            }
            else if (strcmp((const char*)key, "key") == 0) {
                int c = atoi((const char*)val);
                return (uint8_t)c;
            }
        }
    }
    return 0;
}

uint8_t keybuf_get() {
    uint8_t c = 0;
    if (keybuf.delay_count == 0) {
        keybuf.delay_count = keybuf.key_delay;
        c = _keybuf_next();
        if (c != 0) {
            /* check for special ${:} command */
            if (((c == '$') || (c == '#')) && (_keybuf_peek() == '{')) {
                c = _keybuf_parse_cmd();
            }
            /* replace /n with 0x0D */
            if (c == 0x0A) {
                c = 0x0D;
            }
        }
    }
    else {
        keybuf.delay_count--;
    }
    return c;
}
#endif /* COMMON_IMPL */
