#include "sokol_fetch.h"
#include "sokol_app.h"
#include "fs.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define FS_EXT_SIZE (16)
#define FS_PATH_SIZE (256)
#define FS_MAX_SIZE (1024 * 1024)

typedef struct {
    char buf[FS_PATH_SIZE];
    size_t len;
} fs_path_t;

typedef struct {
    fs_path_t path;
    fs_result_t result;
    uint8_t* ptr;
    size_t size;
    uint8_t buf[FS_MAX_SIZE + 1];
} fs_slot_t;

typedef struct {
    bool valid;
    fs_slot_t slots[FS_NUM_SLOTS];
} fs_state_t;
static fs_state_t state;

void fs_init(void) {
    memset(&state, 0, sizeof(state));
    state.valid = true;
    sfetch_setup(&(sfetch_desc_t){
        .max_requests = 1,
        .num_channels = FS_NUM_SLOTS,
        .num_lanes = 1,
    });
}

void fs_dowork(void) {
    assert(state.valid);
    sfetch_dowork();
}

static void fs_path_reset(fs_path_t* path) {
    path->len = 0;
}

static void fs_path_append(fs_path_t* path, const char* str) {
    char c;
    while ((c = *str++) && (path->len < (FS_MAX_SIZE-1))) {
        path->buf[path->len++] = c;
    }
    path->buf[path->len] = 0;
}

static void fs_path_extract_extension(fs_path_t* path, char* buf, size_t buf_size) {
    const char* ext = strrchr(path->buf, '.');
    if (ext) {
        size_t i = 0;
        char c = 0;
        while ((c = *++ext) && (i < (buf_size-1))) {
            buf[i] = tolower(c);
            i++;
        }
        buf[i] = 0;
    }
}

static void fs_copy_filename_and_ext(fs_slot_t* slot, const char* path) {
    slot->fname[0] = 0;
    slot->ext[0] = 0;
    const char* str = path;
    const char* slash = strrchr(str, '/');
    if (slash) {
        str = slash+1;
        slash = strrchr(str, '\\');
        if (slash) {
            str = slash+1;
        }
    }
    strncpy(slot->fname, str, sizeof(slot->fname));
    slot->fname[sizeof(slot->fname)-1] = 0;
    const char* ext = strrchr(str, '.');
    if (ext) {
        int i = 0;
        char c = 0;
        while ((c = *++ext) && (i < (FS_EXT_SIZE-1))) {
            slot->ext[i] = tolower(c);
            i++;
        }
        slot->ext[i] = 0;
    }
}

// http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
static const unsigned char fs_base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static bool fs_base64_decode(fs_slot_t* slot, const char* src) {
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
    if (olen >= (int)sizeof(slot->buf)) {
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
            slot->buf[slot->size++] = (block[0] << 2) | (block[1] >> 4);
            slot->buf[slot->size++] = (block[1] << 4) | (block[2] >> 2);
            slot->buf[slot->size++] = (block[2] << 6) | block[3];
            if (pad > 0) {
                if (pad <= 2) {
                    slot->size -= pad;
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

bool fs_ext(size_t slot_index, const char* ext) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    char buf[FS_EXT_SIZE];
    if (fs_path_extract_extension(&state.slots[slot_index].path)) {
        return 0 == strcmp(ext, buf);
    }
    else {
        return false;
    }
}

const char* fs_filename(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    return state.slots[slot_index].fname;
}

void fs_reset(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    slot->result = FS_RESULT_IDLE;
    slot->ptr = 0;
    slot->size = 0;
}

void fs_load_mem(size_t slot_index, const char* path, fs_range_t data) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    assert(data.ptr && (data.size > 0));
    fs_reset(slot_index);
    fs_slot_t* slot = &state.slots[slot_index];
    if ((data.size > 0) && (data.size <= FS_MAX_SIZE)) {
        fs_copy_filename_and_ext(slot, path);
        slot->result = FS_RESULT_SUCCESS;
        slot->size = data.size;
        slot->ptr = slot->buf;
        memcpy(slot->ptr, data.ptr, data.size);
        /* zero-terminate in case this is a text file */
        slot->ptr[slot->size] = 0;
    }
    else {
        slot->result = FS_RESULT_FAILED;
    }
}

bool fs_load_base64(size_t slot_index, const char* name, const char* payload) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_reset(slot_index);
    fs_slot_t* slot = &state.slots[slot_index];
    fs_copy_filename_and_ext(slot, name);
    if (fs_base64_decode(slot, payload)) {
        slot->result = FS_RESULT_SUCCESS;
        slot->ptr = slot->buf;
        return true;
    }
    else {
        slot->result = FS_RESULT_FAILED;
        return false;
    }
}

static void fs_fetch_callback(const sfetch_response_t* response) {
    assert(state.valid);
    size_t slot_index = *(size_t*)response->user_data;
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    if (response->fetched) {
        slot->result = FS_RESULT_SUCCESS;
        slot->ptr = (uint8_t*)response->data.ptr;
        slot->size = response->data.size;
        assert(slot->size < sizeof(slot->buf));
        // in case it's a text file, zero-terminate the data
        slot->buf[slot->size] = 0;
    }
    else if (response->failed) {
        slot->result = FS_RESULT_FAILED;
    }
}

#if defined(__EMSCRIPTEN__)
static void fs_emsc_dropped_file_callback(const sapp_html5_fetch_response* response) {
    size_t slot_index = *(size_t*)response->user_data;
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    if (response->succeeded) {
        slot->result = FS_RESULT_SUCCESS;
        slot->ptr = response->data.ptr;
        slot->size = response->data.size;
        assert(slot->size < sizeof(slot->buf));
        // in case it's a text file, zero-terminate the data
        slot->buf[slot->size] = 0;
    }
    else if (response->failed) {
        slot->result = FS_RESULT_FAILED;
    }
}
#endif

void fs_start_load_file(size_t slot_index, const char* path) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_reset(slot_index);
    fs_slot_t* slot = &state.slots[slot_index];
    fs_copy_filename_and_ext(slot, path);
    slot->result = FS_RESULT_PENDING;
    sfetch_send(&(sfetch_request_t){
        .path = path,
        .channel = slot_index,
        .callback = fs_fetch_callback,
        .buffer = { .ptr = slot->buf, .size = FS_MAX_SIZE },
        .user_data = { .ptr = &slot_index, .size = sizeof(slot_index) },
    });
}

void fs_start_load_dropped_file(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_reset(slot_index);
    fs_slot_t* slot = &state.slots[slot_index];
    const char* path = sapp_get_dropped_file_path(0);
    fs_copy_filename_and_ext(slot, path);
    slot->result = FS_RESULT_PENDING;
    #if defined(__EMSCRIPTEN__)
        sapp_html5_fetch_dropped_file(&(sapp_html5_fetch_request){
            .dropped_file_index = 0,
            .callback = fs_emsc_dropped_file_callback,
            .buffer = { .ptr = state.buf, .size = FS_MAX_SIZE },
            .user_data = { .ptr = &slot_index, .size = sizeof(slot_index) },
        });
    #else
        fs_start_load_file(slot_index, path);
    #endif
}

fs_result_t fs_result(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    return state.slots[slot_index].result;
}

bool fs_success(size_t slot_index) {
    return fs_result(slot_index) == FS_RESULT_SUCCESS;
}

bool fs_failed(size_t slot_index) {
    return fs_result(slot_index) == FS_RESULT_FAILED;
}

bool fs_pending(size_t slot_index) {
    return fs_result(slot_index) == FS_RESULT_PENDING;
}

fs_range_t fs_data(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    if (slot->result == FS_RESULT_SUCCESS) {
        return (fs_range_t){ .ptr = slot->ptr, .size = slot->size };
    }
    else {
        return (fs_range_t){0};
    }
}

#if defined(__APPLE__)
static const char* fs_snapshot_path(size_t snapshot_index, char* buf, size_t buf_size) {

}
#endif
bool fs_save_snapshot(size_t snapshot_index, fs_range_t data) {


}
