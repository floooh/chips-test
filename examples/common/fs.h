#pragma once
/*
    Simple file access functions.
*/
void fs_init(void);
void fs_dowork(void);
void fs_start_load_file(const char* path);
void fs_start_load_dropped_file(void);
bool fs_load_base64(const char* name, const char* payload);
void fs_load_mem(const char* path, const uint8_t* ptr, uint32_t size);
uint32_t fs_size(void);
const uint8_t* fs_ptr(void);
void fs_free(void);
bool fs_ext(const char* str);

/*== IMPLEMENTATION ==========================================================*/
#ifdef COMMON_IMPL
#include "sokol_fetch.h"
#include "sokol_app.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define FS_EXT_SIZE (16)
#define FS_MAX_SIZE (1024 * 1024)

typedef struct {
    bool valid;
    char ext[FS_EXT_SIZE];
    uint8_t* ptr;
    uint32_t size;
    uint8_t buf[FS_MAX_SIZE + 1];
} fs_state_t;
static fs_state_t fs;

void fs_init(void) {
    memset(&fs, 0, sizeof(fs));
    fs.valid = true;
    sfetch_setup(&(sfetch_desc_t){
        .max_requests = 1,
        .num_channels = 1,
        .num_lanes = 1,
    });
}

void fs_dowork(void) {
    assert(fs.valid);
    sfetch_dowork();
}

static void fs_copy_ext(const char* path) {
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

// http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
static const unsigned char fs_base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static bool fs_base64_decode(const char* src) {
    int len = (int)strlen(src);

    uint8_t dtable[256];
    memset(dtable, 0x80, sizeof(dtable));
    for (int i = 0; i < (int)sizeof(fs_base64_table)-1; i++) {
        dtable[fs_base64_table[i]] = (uint8_t)i;
    }
    dtable['='] = 0;

    int count = 0;
    for (int i = 0; i < len; i++) {
        if (dtable[(int)src[i]] != 0x80) {
            count++;
        }
    }

    // input length must be multiple of 4
    if ((count == 0) || (count & 3)) {
        return false;
    }

    // output length
    int olen = (count / 4) * 3;
    if (olen >= (int)sizeof(fs.buf)) {
        return false;
    }

    // decode loop
    count = 0;
    int pad = 0;
    uint8_t block[4];
    for (int i = 0; i < len; i++) {
        uint8_t tmp = dtable[(int)src[i]];
        if (tmp == 0x80) {
            continue;
        }
        if (src[i] == '=') {
            pad++;
        }
        block[count] = tmp;
        count++;
        if (count == 4) {
            count = 0;
            fs.buf[fs.size++] = (block[0] << 2) | (block[1] >> 4);
            fs.buf[fs.size++] = (block[1] << 4) | (block[2] >> 2);
            fs.buf[fs.size++] = (block[2] << 6) | block[3];
            if (pad > 0) {
                if (pad <= 2) {
                    fs.size -= pad;
                }
                else {
                    // invalid padding
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

bool fs_ext(const char* ext) {
    assert(fs.valid);
    return 0 == strcmp(ext, fs.ext);
}

void fs_free(void) {
    assert(fs.valid);
    memset(&fs, 0, sizeof(fs));
}

void fs_load_mem(const char* path, const uint8_t* ptr, uint32_t size) {
    assert(fs.valid);
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

bool fs_load_base64(const char* name, const char* payload) {
    assert(fs.valid);
    fs_free();
    fs_copy_ext(name);
    if (fs_base64_decode(payload)) {
        fs.ptr = fs.buf;
        return true;
    }
    else {
        return false;
    }
}

static void fs_fetch_callback(const sfetch_response_t* response) {
    assert(fs.valid);
    if (response->fetched) {
        fs.ptr = fs.buf;
        fs.size = response->fetched_size;
        assert(fs.size < sizeof(fs.buf));
        // in case it's a text file, zero-terminate the data
        fs.buf[fs.size] = 0;
    }
    // FIXME: signal failure
}

#if defined(__EMSCRIPTEN__)
static void fs_emsc_dropped_file_callback(const sapp_html5_fetch_response* response) {
    if (response->succeeded) {
        fs.ptr = fs.buf;
        fs.size = response->fetched_size;
        assert(fs.size < sizeof(fs.buf));
        // in case it's a text file, zero-terminate the data
        fs.buf[fs.size] = 0;
    }
    // FIXME: signal failure
}
#endif

void fs_start_load_file(const char* path) {
    assert(fs.valid);
    fs_free();
    fs_copy_ext(path);
    sfetch_send(&(sfetch_request_t){
        .path = path,
        .callback = fs_fetch_callback,
        .buffer_ptr = fs.buf,
        .buffer_size = FS_MAX_SIZE,
    });
}

void fs_start_load_dropped_file(void) {
    assert(fs.valid);
    fs_free();
    const char* path = sapp_get_dropped_file_path(0);
    fs_copy_ext(path);
    #if defined(__EMSCRIPTEN__)
        sapp_html5_fetch_dropped_file(&(sapp_html5_fetch_request){
            .dropped_file_index = 0,
            .callback = fs_emsc_dropped_file_callback,
            .buffer_ptr = fs.buf,
            .buffer_size = FS_MAX_SIZE
        });
    #else
        fs_start_load_file(path);
    #endif
}

const uint8_t* fs_ptr(void) {
    assert(fs.valid);
    return fs.ptr;
}

uint32_t fs_size(void) {
    assert(fs.valid);
    return fs.size;
}

#endif /* COMMON_IMPL */
