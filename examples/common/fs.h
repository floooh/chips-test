#pragma once
/*
    Simple file access functions.
*/
extern void fs_init(void);
extern bool fs_load_file(const char* path);
extern void fs_load_mem(const char* path, const uint8_t* ptr, uint32_t size);
extern uint32_t fs_size(void);
extern const uint8_t* fs_ptr(void);
extern void fs_free(void);
extern bool fs_ext(const char* str);

/*== IMPLEMENTATION ==========================================================*/
#ifdef COMMON_IMPL
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#if !defined(__EMSCRIPTEN__)
#include <stdio.h>
#else
#include <emscripten/emscripten.h>
#endif

#define FS_EXT_SIZE (16)
#define FS_MAX_SIZE (1024 * 1024)
typedef struct {
    char ext[FS_EXT_SIZE];
    uint8_t* ptr;
    uint32_t size;
    uint8_t buf[FS_MAX_SIZE + 1];
} fs_state;
static fs_state fs;

void fs_copy_ext(const char* path) {
    fs.ext[0] = 0;
    const char* str = path;
    const char* slash = strrchr(str, '/');
    if (slash) {
        str = slash+1;
    }
    const char* ext = strrchr(str, '.');
    if (ext) {
        int i = 0;
        char c = 0;
        while ((c = *++ext) && (i < (FS_EXT_SIZE-1))) {
            fs.ext[i] = tolower(c);
            i++;
        }
        fs.ext[i] = 0;
    }
}

bool fs_ext(const char* ext) {
    return 0 == strcmp(ext, fs.ext);
}

void fs_free(void) {
    memset(&fs, 0, sizeof(fs));
}

void fs_load_mem(const char* path, const uint8_t* ptr, uint32_t size) {
    fs_free();
    if ((size > 0) && (size <= FS_MAX_SIZE)) {
        fs_copy_ext(path);
        fs.size = size;
        fs.ptr = fs.buf;
        memcpy(fs.ptr, ptr, size);
        /* zero-terminate in case this is a text file */
        fs.ptr[fs.size] = 0;
    }
}

#if !defined(__EMSCRIPTEN__)
bool fs_load_file(const char* path) {
    fs_free();
    fs_copy_ext(path);
    FILE* fp = fopen(path, "rb");
    bool success = false;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        if (size <= FS_MAX_SIZE) {
            fs.size = size;
            fseek(fp, 0, SEEK_SET);
            if (fs.size > 0) {
                fs.ptr = fs.buf;
                uint32_t res = (int) fread(fs.ptr, 1, fs.size, fp); (void)res;
                success = (res == fs.size);
            }
            fclose(fp);
            /* zero-terminate in case this is a text file */
            fs.ptr[fs.size] = 0;
        }
    }
    return success;
}
#else
EMSCRIPTEN_KEEPALIVE void emsc_load_data(const char* path, const uint8_t* ptr, int size) {
    fs_load_mem(path, ptr, size);
}

EM_JS(void, emsc_fs_init, (void), {
    console.log("fs.h: registering Module['ccall']");
    Module['ccall'] = ccall;
});

EM_JS(void, emsc_load_file, (const char* path_cstr), {
    var path = UTF8ToString(path_cstr);
    var req = new XMLHttpRequest();
    req.open("GET", path);
    req.responseType = "arraybuffer";
    req.onload = function(e) {
        var uint8Array = new Uint8Array(req.response);
        var res = ccall('emsc_load_data',
            'int',
            ['string', 'array', 'number'],
            [path, uint8Array, uint8Array.length]);
    };
    req.send();
});

/* NOTE: this is loading the data asynchronously, need to check fs_ptr()
   whether the data has actually been loaded!
*/
bool fs_load_file(const char* path) {
    fs_free();
    emsc_load_file(path);
    return true;
}
#endif

void fs_init(void) {
    memset(&fs, 0, sizeof(fs));
    #if defined(__EMSCRIPTEN__)
    emsc_fs_init();
    #endif
}

const uint8_t* fs_ptr(void) {
    return fs.ptr;
}

uint32_t fs_size(void) {
    return fs.size;
}

#endif /* COMMON_IMPL */
