#include "webapi.h"
#include <assert.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

static struct {
    webapi_interface_t funcs;
} state;

void webapi_init(const webapi_desc_t* desc) {
    assert(desc);
    state.funcs = desc->funcs;
}

#if defined(__EMSCRIPTEN__)

EMSCRIPTEN_KEEPALIVE void* webapi_alloc(int size) {
    return malloc((size_t)size);
}

EMSCRIPTEN_KEEPALIVE void webapi_free(void* ptr) {
    free(ptr);
}

EMSCRIPTEN_KEEPALIVE void webapi_boot(void) {
    assert(state.funcs.boot);
    state.funcs.boot();
}

EMSCRIPTEN_KEEPALIVE void webapi_reset(void) {
    assert(state.funcs.reset);
    state.funcs.reset();
}

EMSCRIPTEN_KEEPALIVE void webapi_quickload(void* ptr, int size) {
    if (ptr && (size > 0)) {
        assert(state.funcs.quickload);
        const chips_range_t data = { .ptr = ptr, .size = (size_t) size };
        state.funcs.quickload(data);
    }
}

#endif // __EMSCRIPTEN__
