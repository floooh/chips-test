#include "webapi.h"
#include <assert.h>
#include <string.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#include "gfx.h"

#if defined(EM_JS_DEPS)
EM_JS_DEPS(chips_webapi, "$withStackSave,$stringToUTF8OnStack");
#endif

static struct {
    bool dbg_connect_requested;
} before_init_state;

static struct {
    bool inited;
    webapi_interface_t funcs;
} state;

void webapi_init(const webapi_desc_t* desc) {
    assert(desc);
    state.inited = true;
    state.funcs = desc->funcs;
    if (before_init_state.dbg_connect_requested && state.funcs.dbg_connect) {
        state.funcs.dbg_connect();
    }

    // replace webapi_input() export with a JS shim which takes care of string marshalling
    #if defined(__EMSCRIPTEN__)
    EM_ASM({
        const cfunc = Module["_webapi_input"];
        Module["_webapi_input"] = (text) => {
            withStackSave(() => cfunc(stringToUTF8OnStack(text)));
        }
    });
    #endif
}

#if defined(__EMSCRIPTEN__)

EM_JS(void, webapi_js_event_stopped, (int stop_reason, uint16_t addr), {
    console.log("webapi_js_event_stopped()");
    if (Module["webapi_onStopped"]) {
        Module["webapi_onStopped"](stop_reason, addr);
    } else {
        console.log("no Module.webapi_onStopped function");
    }
});

EM_JS(void, webapi_js_event_continued, (), {
    console.log("webapi_js_event_continued()");
    if (Module["webapi_onContinued"]) {
        Module["webapi_onContinued"]();
    } else {
        console.log("no Module.webapi_onContinued function");
    }
});

EM_JS(void, webapi_js_event_reboot, (), {
    console.log("webapi_js_event_reboot()");
    if (Module["webapi_onReboot"]) {
        Module["webapi_onReboot"]();
    } else {
        console.log("no Module.webapi_onReboot function");
    }
});

EM_JS(void, webapi_js_event_reset, (), {
    console.log("webapi_js_event_reset()");
    if (Module["webapi_onReset"]) {
        Module["webapi_onReset"]();
    } else {
        console.log("no Module.webapi_onReset function");
    }
});

EMSCRIPTEN_KEEPALIVE void webapi_dbg_connect(void) {
    if (state.inited) {
        if (state.funcs.dbg_connect) {
            state.funcs.dbg_connect();
        }
    } else {
        before_init_state.dbg_connect_requested = true;
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_dbg_disconnect(void) {
    if (state.inited && state.funcs.dbg_disconnect) {
        state.funcs.dbg_disconnect();
    }
}

EMSCRIPTEN_KEEPALIVE void* webapi_alloc(int size) {
    return malloc((size_t)size);
}

EMSCRIPTEN_KEEPALIVE void webapi_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
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

EMSCRIPTEN_KEEPALIVE bool webapi_ready(void) {
    if (state.inited && state.funcs.ready) {
        return state.funcs.ready();
    } else {
        return false;
    }
}

EMSCRIPTEN_KEEPALIVE bool webapi_load(void* ptr, int size) {
    if (state.inited && state.funcs.load && ptr && ((size_t)size > sizeof(webapi_fileheader_t))) {
        const webapi_fileheader_t* hdr = (webapi_fileheader_t*)ptr;
        if ((hdr->magic[0] != 'C') || (hdr->magic[1] != 'H') || (hdr->magic[2] != 'I') || (hdr->magic[3] != 'P')) {
            return false;
        }
        return state.funcs.load((chips_range_t){ .ptr = ptr, .size = (size_t)size });
    }
    return false;
}

EMSCRIPTEN_KEEPALIVE bool webapi_load_snapshot(size_t index) {
    if (state.funcs.load_snapshot != NULL) {
        return state.funcs.load_snapshot(index);
    } else {
        return false;
    }
}

EMSCRIPTEN_KEEPALIVE void webapi_save_snapshot(size_t index) {
    if (state.funcs.save_snapshot != NULL) {
        state.funcs.save_snapshot(index);
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

// request a disassembly, returns ptr to heap-allocated array of 'num_lines' webapi_dasm_line_t structs which must be freed with webapi_free()
// NOTE: may return 0!
EMSCRIPTEN_KEEPALIVE webapi_dasm_line_t* webapi_dbg_request_disassembly(uint16_t addr, int offset_lines, int num_lines) {
    if (num_lines <= 0) {
        return 0;
    }
    if (state.inited && state.funcs.dbg_request_disassembly) {
        webapi_dasm_line_t* out_lines = calloc((size_t)num_lines, sizeof(webapi_dasm_line_t));
        state.funcs.dbg_request_disassembly(addr, offset_lines, num_lines, out_lines);
        return out_lines;
    } else {
        return 0;
    }
}

// reads a memory chunk, returns heap-allocated buffer which must be freed with webapi_free()
// NOTE: may return 0!
EMSCRIPTEN_KEEPALIVE uint8_t* webapi_dbg_read_memory(uint16_t addr, int num_bytes) {
    if (state.inited && state.funcs.dbg_read_memory) {
        uint8_t* ptr = calloc((size_t)num_bytes, 1);
        state.funcs.dbg_read_memory(addr, num_bytes, ptr);
        return ptr;
    } else {
        return 0;
    }
}

EMSCRIPTEN_KEEPALIVE bool webapi_input(char* text) {
    if (state.funcs.input != NULL && text != NULL) {
        state.funcs.input(text);
        return true;
    } else {
        return false;
    }
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

void webapi_event_reboot(void) {
    #if defined(__EMSCRIPTEN__)
        webapi_js_event_reboot();
    #endif
}

void webapi_event_reset(void) {
    #if defined(__EMSCRIPTEN__)
        webapi_js_event_reset();
    #endif
}