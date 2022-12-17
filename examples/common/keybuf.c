#include "keybuf.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define KEYBUF_MAX_KEYS (64 * 1024)
typedef struct {
    bool valid;
    int cur_pos;
    int cur_delay_time;
    int key_delay_time;
    uint8_t buf[KEYBUF_MAX_KEYS];
} keybuf_state_t;
static keybuf_state_t state;

void keybuf_init(const keybuf_desc_t* desc) {
    state = (keybuf_state_t) {
        .valid = true,
        .key_delay_time = desc->key_delay_frames * 16667,
    };
}

void keybuf_put(const char* text) {
    assert(state.valid);
    if (!text) {
        return;
    }
    state.cur_delay_time = 0;
    int len = (int) strlen(text);
    if ((len+1) < KEYBUF_MAX_KEYS) {
        strcpy((char*)state.buf, text);
    }
    else {
        state.buf[0] = 0;
    }
    state.cur_pos = 0;
}

static uint8_t _keybuf_peek(void) {
    if (state.cur_pos < KEYBUF_MAX_KEYS) {
        return state.buf[state.cur_pos];
    }
    else {
        return 0;
    }
}

static uint8_t _keybuf_next(void) {
    uint8_t c = _keybuf_peek();
    if (0 != c) {
        state.cur_pos++;
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
                state.cur_delay_time = atoi((const char*)val) * 16667;
                return 0;
            }
            else if (strcmp((const char*)key, "delay") == 0) {
                state.key_delay_time = atoi((const char*)val) * 16667;
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

uint8_t keybuf_get(uint32_t frame_time_us) {
    assert(state.valid);
    uint8_t c = 0;
    if (state.cur_delay_time <= 0) {
        state.cur_delay_time = state.key_delay_time;
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
        state.cur_delay_time -= (int) frame_time_us;
    }
    return c;
}
