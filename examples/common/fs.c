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
EMSCRIPTEN_KEEPALIVE void emsc_load_data(const uint8_t* ptr, int size) {
    fs_load_mem(ptr, size);
}

EM_JS(void, emsc_load_file, (const char* path_cstr), {
    var path = Pointer_stringify(path_cstr);
    var req = new XMLHttpRequest();
    req.open("GET", file);
    req.responseType = "arraybuffer";
    req.onload = function(e) {
        var uint8Array = new Uint8Array(req.response);
        var res = Module.ccall('emsc_load_data',
            'int',
            ['array', 'number'],
            [uint8Array, uint8Array.length]);
    };
    req.send();
});

/* NOTE: this is loading the data asynchronously, need to check fs_ptr()
   whether the data has actually been loaded!
*/
bool fs_load_file(const char* path) {
    fs_free();
    emsc_load_file(path);
    return false;
}
#endif

void fs_init(void) {
    // FIXME?
}

const uint8_t* fs_ptr(void) {
    return _fs_ptr;
}

uint32_t fs_size(void) {
    return _fs_size;
}
