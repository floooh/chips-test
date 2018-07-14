#if !defined(__EMSCRIPTEN__)
#include <stdio.h>
#else
#include <emscripten/emscripten.h>
#endif
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static uint8_t* _fs_ptr;
static uint32_t _fs_size;

void fs_free(void) {
    if (_fs_ptr) {
        free((void*)_fs_ptr);
        _fs_ptr = 0;
        _fs_size = 0;
    }
}

void fs_load_mem(const uint8_t* ptr, uint32_t size) {
    fs_free();
    if (size > 0) {
        _fs_size = size;
        _fs_ptr = malloc(size);
        memcpy(_fs_ptr, ptr, size);
    }
}

#if !defined(__EMSCRIPTEN__)
bool fs_load_file(const char* path) {
    fs_free();
    FILE* fp = fopen(path, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        _fs_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (_fs_size > 0) {
            _fs_ptr = malloc(_fs_size);
            fread(_fs_ptr, 1, _fs_size, fp);
        }
        fclose(fp);
        return true;
    }
    else {
        return false;
    }
}
#else
bool fs_load_file(const char* path) {
    fs_free();
    return false;
}

EMSCRIPTEN_KEEPALIVE void emsc_loadfile(const uint8_t* ptr, int size) {
    printf("emsc_loadfile(%p, %d)\n", ptr, size);
    fs_load_mem(ptr, size);
}

EM_JS(void, emsc_check_url_params, (), {
    var params = new URLSearchParams(window.location.search);
    if (params.has("file")) {
        var file = params.get("file");
        console.log("loading file: ", file);
        var req = new XMLHttpRequest();
        req.open("GET", file);
        req.responseType = "arraybuffer";
        req.onload = function(e) {
            var uint8Array = new Uint8Array(req.response);
            console.log("loaded!");
            var res = Module.ccall('emsc_loadfile',
                'int',
                ['array', 'number'],
                [uint8Array, uint8Array.length]);
        };
        req.send();
    };
});
#endif

void fs_init(void) {
    #if defined(__EMSCRIPTEN__)
    emsc_check_url_params();
    #endif
}

const uint8_t* fs_ptr(void) {
    return _fs_ptr;
}

uint32_t fs_size(void) {
    return _fs_size;
}
