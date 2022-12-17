#include "sokol_fetch.h"
#include "sokol_app.h"
#include "fs.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define FS_EXT_SIZE (16)
#define FS_FNAME_SIZE (32)
#define FS_MAX_SIZE (1024 * 1024)

typedef struct {
    bool valid;
    char fname[FS_FNAME_SIZE];
    char ext[FS_EXT_SIZE];
    uint8_t* ptr;
    uint32_t size;
    uint8_t buf[FS_MAX_SIZE + 1];
} fs_state_t;
static fs_state_t state;

void fs_init(void) {
    memset(&state, 0, sizeof(state));
    state.valid = true;
    sfetch_setup(&(sfetch_desc_t){
        .max_requests = 1,
        .num_channels = 1,
        .num_lanes = 1,
    });
}

void fs_dowork(void) {
    assert(state.valid);
    sfetch_dowork();
}

static void fs_strcpy(char* dst, const char* src, size_t buf_size) {
    strncpy(dst, src, buf_size);
    state.fname[buf_size-1] = 0;
}

static void fs_copy_filename_and_ext(const char* path) {
    state.fname[0] = 0;
    state.ext[0] = 0;
    const char* str = path;
    const char* slash = strrchr(str, '/');
    if (slash) {
        str = slash+1;
        slash = strrchr(str, '\\');
        if (slash) {
            str = slash+1;
        }
    }
    fs_strcpy(state.fname, str, sizeof(state.fname));
    const char* ext = strrchr(str, '.');
    if (ext) {
        int i = 0;
        char c = 0;
        while ((c = *++ext) && (i < (FS_EXT_SIZE-1))) {
            state.ext[i] = tolower(c);
            i++;
        }
        state.ext[i] = 0;
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
    if (olen >= (int)sizeof(state.buf)) {
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
            state.buf[state.size++] = (block[0] << 2) | (block[1] >> 4);
            state.buf[state.size++] = (block[1] << 4) | (block[2] >> 2);
            state.buf[state.size++] = (block[2] << 6) | block[3];
            if (pad > 0) {
                if (pad <= 2) {
                    state.size -= pad;
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
    assert(state.valid);
    return 0 == strcmp(ext, state.ext);
}

const char* fs_filename(void) {
    return state.fname;
}

void fs_reset(void) {
    assert(state.valid);
    state.ptr = 0;
    state.size = 0;
}

void fs_load_mem(const char* path, const uint8_t* ptr, uint32_t size) {
    assert(state.valid);
    fs_reset();
    if ((size > 0) && (size <= FS_MAX_SIZE)) {
        fs_copy_filename_and_ext(path);
        state.size = size;
        state.ptr = state.buf;
        memcpy(state.ptr, ptr, size);
        /* zero-terminate in case this is a text file */
        state.ptr[state.size] = 0;
    }
}

bool fs_load_base64(const char* name, const char* payload) {
    assert(state.valid);
    fs_reset();
    fs_copy_filename_and_ext(name);
    if (fs_base64_decode(payload)) {
        state.ptr = state.buf;
        return true;
    }
    else {
        return false;
    }
}

static void fs_fetch_callback(const sfetch_response_t* response) {
    assert(state.valid);
    if (response->fetched) {
        state.ptr = state.buf;
        state.size = (uint32_t)response->data.size;
        assert(state.size < sizeof(state.buf));
        // in case it's a text file, zero-terminate the data
        state.buf[state.size] = 0;
    }
    // FIXME: signal failure
}

#if defined(__EMSCRIPTEN__)
static void fs_emsc_dropped_file_callback(const sapp_html5_fetch_response* response) {
    if (response->succeeded) {
        state.ptr = state.buf;
        state.size = response->data.size;
        assert(state.size < sizeof(state.buf));
        // in case it's a text file, zero-terminate the data
        state.buf[state.size] = 0;
    }
    // FIXME: signal failure
}
#endif

void fs_start_load_file(const char* path) {
    assert(state.valid);
    fs_reset();
    fs_copy_filename_and_ext(path);
    sfetch_send(&(sfetch_request_t){
        .path = path,
        .callback = fs_fetch_callback,
        .buffer = { .ptr = state.buf, .size = FS_MAX_SIZE },
    });
}

void fs_start_load_dropped_file(void) {
    assert(state.valid);
    fs_reset();
    const char* path = sapp_get_dropped_file_path(0);
    fs_copy_filename_and_ext(path);
    #if defined(__EMSCRIPTEN__)
        sapp_html5_fetch_dropped_file(&(sapp_html5_fetch_request){
            .dropped_file_index = 0,
            .callback = fs_emsc_dropped_file_callback,
            .buffer = { .ptr = state.buf, .size = FS_MAX_SIZE }
        });
    #else
        fs_start_load_file(path);
    #endif
}

const uint8_t* fs_ptr(void) {
    assert(state.valid);
    return state.ptr;
}

uint32_t fs_size(void) {
    assert(state.valid);
    return state.size;
}
