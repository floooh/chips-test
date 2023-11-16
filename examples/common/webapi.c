#include "webapi.h"
#include <assert.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#include "gfx.h"

static struct {
    bool disable_speaker_icon;
} before_init_state;

static struct {
    bool inited;
    webapi_interface_t funcs;
} state;

void webapi_init(const webapi_desc_t* desc) {
    assert(desc);
    state.inited = true;
    state.funcs = desc->funcs;
    if (before_init_state.disable_speaker_icon) {
        gfx_disable_speaker_icon();
    }
}

#if defined(__EMSCRIPTEN__)

EMSCRIPTEN_KEEPALIVE void* webapi_alloc(int size) {
    return malloc((size_t)size);
}

EMSCRIPTEN_KEEPALIVE void webapi_free(void* ptr) {
    free(ptr);
}

EMSCRIPTEN_KEEPALIVE void webapi_boot(void) {
    if (!state.inited) {
        return;
    }
    assert(state.funcs.boot);
    state.funcs.boot();
}

EMSCRIPTEN_KEEPALIVE void webapi_reset(void) {
    if (!state.inited) {
        return;
    }
    assert(state.funcs.reset);
    state.funcs.reset();
}

EMSCRIPTEN_KEEPALIVE void webapi_quickload(void* ptr, int size) {
    if (!state.inited) {
        return;
    }
    if (ptr && (size > 0)) {
        assert(state.funcs.quickload);
        const chips_range_t data = { .ptr = ptr, .size = (size_t) size };
        state.funcs.quickload(data);
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_disable_speaker_icon(void) {
    if (state.inited) {
        gfx_disable_speaker_icon();
    } else {
        before_init_state.disable_speaker_icon = true;
    }
}

#endif // __EMSCRIPTEN__
