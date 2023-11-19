#include "webapi.h"
#include <assert.h>
#include <string.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#include "gfx.h"

static struct {
    bool enable_external_debugger;
} before_init_state;

static struct {
    bool inited;
    webapi_interface_t funcs;
} state;

void webapi_init(const webapi_desc_t* desc) {
    assert(desc);
    state.inited = true;
    state.funcs = desc->funcs;
    if (before_init_state.enable_external_debugger && state.funcs.enable_external_debugger) {
        state.funcs.enable_external_debugger();
    }
}

#if defined(__EMSCRIPTEN__)

EM_JS(void, webapi_js_event_stopped, (int stop_reason, uint16_t addr), {
    console.log("webapi_js_event_stopped()");
    if (Module['webapi_onStopped']) {
        Module['webapi_onStopped'](stop_reason, addr);
    } else {
        console.log("no Module.webapi.onStopped function");
    }
});

EM_JS(void, webapi_js_event_continued, (), {
    console.log("webapi_js_event_continued()");
    if (Module['webapi_onContinued']) {
        Module['webapi_onContinued']();
    } else {
        console.log("no Module.webapi.onContinued function");
    }
});

EMSCRIPTEN_KEEPALIVE void webapi_enable_external_debugger(void) {
    if (state.inited) {
        if (state.funcs.enable_external_debugger) {
            state.funcs.enable_external_debugger();
        }
    } else {
        before_init_state.enable_external_debugger = true;
    }
}

EMSCRIPTEN_KEEPALIVE void* webapi_alloc(int size) {
    return malloc((size_t)size);
}

EMSCRIPTEN_KEEPALIVE void webapi_free(void* ptr) {
    free(ptr);
}

EMSCRIPTEN_KEEPALIVE void webapi_boot(void) {
    if (state.inited && state.funcs.boot) {
        state.funcs.boot();
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_reset(void) {
    if (state.inited && state.funcs.reset) {
        state.funcs.reset();
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_quickload(void* ptr, int size, int start, int stop_on_entry) {
    if (state.inited && state.funcs.quickload && ptr && (size > 0)) {
        const chips_range_t data = { .ptr = ptr, .size = (size_t) size };
        state.funcs.quickload(data, start, stop_on_entry);
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_dbg_add_breakpoint(uint16_t addr) {
    if (state.inited && state.funcs.dbg_add_breakpoint) {
        state.funcs.dbg_add_breakpoint(addr);
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_dbg_remove_breakpoint(uint16_t addr) {
    if (state.inited && state.funcs.dbg_remove_breakpoint) {
        state.funcs.dbg_remove_breakpoint(addr);
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_dbg_break(void) {
    if (state.inited && state.funcs.dbg_break) {
        state.funcs.dbg_break();
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_dbg_continue(void) {
    if (state.inited && state.funcs.dbg_continue) {
        state.funcs.dbg_continue();
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_dbg_step_next(void) {
    if (state.inited && state.funcs.dbg_step_next) {
        state.funcs.dbg_step_next();
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_dbg_step_into(void) {
    if (state.inited && state.funcs.dbg_step_into) {
        state.funcs.dbg_step_into();
    }
}

// return emulator state as JSON-formatted string pointer into WASM heap
EMSCRIPTEN_KEEPALIVE uint16_t* webapi_dbg_cpu_state(void) {
    static webapi_cpu_state_t res;
    if (state.inited && state.funcs.dbg_cpu_state) {
        res = state.funcs.dbg_cpu_state();
    } else {
        memset(&res, 0, sizeof(res));
    }
    return &res.items[0];
}

#endif // __EMSCRIPTEN__

// stop_reason is UI_DBG_STOP_REASON_xxx
void webapi_event_stopped(int stop_reason, uint16_t addr) {
    #if defined(__EMSCRIPTEN__)
        webapi_js_event_stopped(stop_reason, addr);
    #else
        (void)stop_reason; (void)addr;
    #endif
}

void webapi_event_continued(void) {
    #if defined(__EMSCRIPTEN__)
        webapi_js_event_continued();
    #endif
}
