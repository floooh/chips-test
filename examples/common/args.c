#include "args.h"
#include <string.h>
#include <assert.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#define MAX_ARGS (16)
#define BUF_SIZE (MAX_ARGS*128)
static int num_args;
static const char* keys[MAX_ARGS];
static const char* vals[MAX_ARGS];
static int pos;
static char buf[BUF_SIZE];

/* add a key-value-string of the form "key=value" */
static void add_kvp(const char* kvp) {
    const char* key_ptr;
    const char* val_ptr;
    char c;
    const char* src = kvp;
    key_ptr = buf + pos;
    val_ptr = 0;
    while (0 != (c = *src++)) {
        if ((pos+2) < BUF_SIZE) {
            if (c == '=') {
                buf[pos++] = 0;
                val_ptr = buf + pos;
            }
            else if (c > 32) {
                buf[pos++] = c;
            }
        }
    }
    if ((pos+2) < BUF_SIZE) {
        buf[pos++] = 0;
        if (num_args < MAX_ARGS) {
            keys[num_args] = key_ptr;
            vals[num_args] = val_ptr;
            num_args++;
        }
    }
}

/* emscripten helpers to parse browser URL args of the form "key=value" */
#if defined(__EMSCRIPTEN__)
EMSCRIPTEN_KEEPALIVE void args_emsc_add(const char* kvp) {
    add_kvp(kvp);
}

EM_JS(void, args_emsc_parse_url, (), {
    var params = new URLSearchParams(window.location.search).entries();
    for (var p = params.next(); !p.done; p = params.next()) {
        var kvp = p.value[0] + '=' + p.value[1];
        var res = Module.ccall('args_emsc_add', 'void', ['string'], [kvp]);
    }
});
#endif

void args_init(int argc, char* argv[]) {
    #if !defined(__EMSCRIPTEN__)
        for (int i = 1; i < argc; i++) {
            add_kvp(argv[i]);
        }
    #else
        args_emsc_parse_url();
    #endif
}

static int find_key(const char* key) {
    assert(key);
    for (int i = 0; i < num_args; i++) {
        if (keys[i] && (0 == strcmp(keys[i], key))) {
            return i;
        }
    }
    return -1;
}

bool args_has(const char* key) {
    return -1 != find_key(key);
}

const char* args_string(const char* key) {
    if (key) {
        int i = find_key(key);
        if ((-1 != i) && (vals[i])) {
            return vals[i];
        }
    }
    return "INVALID";
}

bool args_bool(const char* key) {
    if (key) {
        const char* val = args_string(key);
        assert(val);
        if (0 == strcmp(val, "true")) {
            return true;
        }
        if (0 == strcmp(val, "yes")) {
            return true;
        }
        if (0 == strcmp(val, "on")) {
            return true;
        }
    }
    return false;
}

