#include "sokol_fetch.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "chips/chips_common.h"
#include "fs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdalign.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#if defined(WIN32)
#include <windows.h>
#endif

#define FS_EXT_SIZE (16)
#define FS_PATH_SIZE (256)
#define FS_MAX_SIZE (2024 * 1024)

typedef struct {
    char cstr[FS_PATH_SIZE];
    size_t len;
    bool clamped;
} fs_path_t;

typedef struct {
    size_t snapshot_index;
    fs_snapshot_load_callback_t callback;
} fs_snapshot_load_context_t;

typedef struct {
    fs_path_t path;
    fs_result_t result;
    uint8_t* ptr;
    size_t size;
    alignas(64) uint8_t buf[FS_MAX_SIZE + 1];
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
        .max_requests = 128,
        .num_channels = FS_NUM_SLOTS,
        .num_lanes = 1,
        .logger.func = slog_func,
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
    const size_t max_len = sizeof(path->cstr) - 1;
    char c;
    while ((c = *str++) && (path->len < max_len)) {
        path->cstr[path->len++] = c;
    }
    path->cstr[path->len] = 0;
    path->clamped = (c != 0);
}

static void fs_path_extract_extension(fs_path_t* path, char* buf, size_t buf_size) {
    const char* tail = strrchr(path->cstr, '\\');
    if (0 == tail) {
        tail = strrchr(path->cstr, '/');
    }
    if (0 == tail) {
        tail = path->cstr;
    }
    const char* ext = strrchr(tail, '.');
    buf[0] = 0;
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
    fs_path_extract_extension(&state.slots[slot_index].path, buf, sizeof(buf));
    return 0 == strcmp(ext, buf);
}

const char* fs_filename(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    return state.slots[slot_index].path.cstr;
}

void fs_reset(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    fs_path_reset(&slot->path);
    slot->result = FS_RESULT_IDLE;
    slot->ptr = 0;
    slot->size = 0;
}

void fs_load_mem(size_t slot_index, const char* path, chips_range_t data) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    assert(data.ptr && (data.size > 0));
    fs_reset(slot_index);
    fs_slot_t* slot = &state.slots[slot_index];
    if ((data.size > 0) && (data.size <= FS_MAX_SIZE)) {
        fs_path_append(&slot->path, path);
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
    fs_path_append(&slot->path, name);
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
    size_t slot_index = (size_t)(uintptr_t)response->user_data;
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    if (response->succeeded) {
        slot->result = FS_RESULT_SUCCESS;
        slot->ptr = (uint8_t*)response->data.ptr;
        slot->size = response->data.size;
        assert(slot->size < sizeof(slot->buf));
        // in case it's a text file, zero-terminate the data
        slot->buf[slot->size] = 0;
    }
    else {
        slot->result = FS_RESULT_FAILED;
    }
}
#endif

void fs_start_load_file(size_t slot_index, const char* path) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_reset(slot_index);
    fs_slot_t* slot = &state.slots[slot_index];
    fs_path_append(&slot->path, path);
    slot->result = FS_RESULT_PENDING;
    sfetch_send(&(sfetch_request_t){
        .path = path,
        .channel = (int)slot_index,
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
    fs_path_append(&slot->path, path);
    slot->result = FS_RESULT_PENDING;
    #if defined(__EMSCRIPTEN__)
        sapp_html5_fetch_dropped_file(&(sapp_html5_fetch_request){
            .dropped_file_index = 0,
            .callback = fs_emsc_dropped_file_callback,
            .buffer = { .ptr = slot->buf, .size = FS_MAX_SIZE },
            .user_data = (void*)(intptr_t)slot_index,
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

chips_range_t fs_data(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    if (slot->result == FS_RESULT_SUCCESS) {
        return (chips_range_t){ .ptr = slot->ptr, .size = slot->size };
    }
    else {
        return (chips_range_t){0};
    }
}

fs_path_t fs_make_snapshot_path(const char* dir, const char* system_name, size_t snapshot_index) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%zu", snapshot_index);
    fs_path_t path = {0};
    fs_path_append(&path, dir);
    fs_path_append(&path, "/chips_");
    fs_path_append(&path, system_name);
    fs_path_append(&path, "_snapshot_");
    fs_path_append(&path, buf);
    return path;
}

#if !defined(__EMSCRIPTEN__)
static void fs_snapshot_fetch_callback(const sfetch_response_t* response) {
    const fs_snapshot_load_context_t* ctx = (fs_snapshot_load_context_t*) response->user_data;
    size_t snapshot_index = ctx->snapshot_index;
    fs_snapshot_load_callback_t callback = ctx->callback;
    if (response->fetched) {
        callback(&(fs_snapshot_response_t){
            .snapshot_index = snapshot_index,
            .result = FS_RESULT_SUCCESS,
            .data = {
                .ptr = (uint8_t*)response->data.ptr,
                .size = response->data.size
            }
        });
    }
    else if (response->failed) {
        callback(&(fs_snapshot_response_t){
            .snapshot_index = snapshot_index,
            .result = FS_RESULT_FAILED,
        });
    }
}
#endif

#if defined (WIN32)
fs_path_t fs_win32_make_snapshot_path_utf8(const char* system_name, size_t snapshot_index) {
    WCHAR wc_tmp_path[1024];
    if (0 == GetTempPathW(sizeof(wc_tmp_path), wc_tmp_path)) {
        return (fs_path_t){0};
    }
    char utf8_tmp_path[2048];
    if (0 == WideCharToMultiByte(CP_UTF8, 0, wc_tmp_path, -1, utf8_tmp_path, sizeof(utf8_tmp_path), NULL, NULL)) {
        return (fs_path_t){0};
    }
    return fs_make_snapshot_path(utf8_tmp_path, system_name, snapshot_index);
}

bool fs_win32_make_snapshot_path_wide(const char* system_name, size_t snapshot_index, WCHAR* out_buf, size_t out_buf_size_bytes) {
    const fs_path_t path = fs_win32_make_snapshot_path_utf8(system_name, snapshot_index);
    if ((path.len == 0) || (path.clamped)) {
        return false;
    }
    if (0 == MultiByteToWideChar(CP_UTF8, 0, path.cstr, -1, out_buf, out_buf_size_bytes)) {
        return false;
    }
    return true;
}

bool fs_save_snapshot(const char* system_name, size_t snapshot_index, chips_range_t data) {
    WCHAR wc_path[1024];
    if (!fs_win32_make_snapshot_path_wide(system_name, snapshot_index, wc_path, sizeof(wc_path)/sizeof(WCHAR))) {
        return false;
    }
    HANDLE fp = CreateFileW(wc_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fp == INVALID_HANDLE_VALUE) {
        return false;
    }
    if (!WriteFile(fp, data.ptr, data.size, NULL, NULL)) {
        CloseHandle(fp);
        return false;
    }
    CloseHandle(fp);
    return true;
}

bool fs_start_load_snapshot(size_t slot_index, const char* system_name, size_t snapshot_index, fs_snapshot_load_callback_t callback) {
    assert(slot_index < FS_NUM_SLOTS);
    assert(system_name && callback);
    fs_path_t path = fs_win32_make_snapshot_path_utf8(system_name, snapshot_index);
    if ((path.len == 0) || path.clamped) {
        return false;
    }
    fs_snapshot_load_context_t context = {
        .snapshot_index = snapshot_index,
        .callback = callback
    };
    fs_slot_t* slot = &state.slots[slot_index];
    sfetch_send(&(sfetch_request_t){
        .path = path.cstr,
        .channel = slot_index,
        .callback = fs_snapshot_fetch_callback,
        .buffer = { .ptr = slot->buf, .size = FS_MAX_SIZE },
        .user_data = { .ptr = &context, .size = sizeof(context) }
    });
    return true;
}

#elif defined(__EMSCRIPTEN__)

EM_JS(void, fs_js_save_snapshot, (const char* system_name_cstr, int snapshot_index, void* bytes, int num_bytes), {
    const db_name = 'chips';
    const db_store_name = 'store';
    const system_name = UTF8ToString(system_name_cstr);
    console.log('fs_js_save_snapshot: called with', system_name, snapshot_index);
    let open_request;
    try {
        open_request = window.indexedDB.open(db_name, 1);
    } catch (e) {
        console.log('fs_js_save_snapshot: failed to open IndexedDB with ' + e);
        return;
    }
    open_request.onupgradeneeded = () => {
        console.log('fs_js_save_snapshot: creating db');
        let db = open_request.result;
        db.createObjectStore(db_store_name);
    };
    open_request.onsuccess = () => {
        console.log('fs_js_save_snapshot: onsuccess');
        let db = open_request.result;
        let transaction = db.transaction([db_store_name], 'readwrite');
        let file = transaction.objectStore(db_store_name);
        let key = system_name + '_' + snapshot_index;
        let blob = HEAPU8.subarray(bytes, bytes + num_bytes);
        let put_request = file.put(blob, key);
        put_request.onsuccess = () => {
            console.log('fs_js_save_snapshot:', key, 'successfully stored')
        };
        put_request.onerror = () => {
            console.log('fs_js_save_snapshot: FAILED to store', key);
        };
        transaction.onerror = () => {
            console.log('fs_js_save_snapshot: transaction onerror');
        };
    };
    open_request.onerror = () => {
        console.log('fs_js_save_snapshot: open_request onerror');
    }
});

EM_JS(void, fs_js_load_snapshot, (const char* system_name_cstr, int snapshot_index, fs_snapshot_load_context_t* context), {
    const db_name = 'chips';
    const db_store_name = 'store';
    const system_name = UTF8ToString(system_name_cstr);
    let open_request;
    try {
        open_request = window.indexedDB.open(db_name, 1);
    } catch (e) {
        console.log('fs_js_load_snapshot: failed to open IndexedDB with ' + e);
    }
    open_request.onupgradeneeded = () => {
        console.log('fs_js_load_snapshot: creating db');
        let db = open_request.result;
        db.createObjectStore(db_store_name);
    };
    open_request.onsuccess = () => {
        let db = open_request.result;
        let transaction;
        try {
            transaction = db.transaction([db_store_name], 'readwrite');
        } catch (e) {
            console.log('fs_js_load_snapshot: db.transaction failed with', e);
            return;
        };
        let file = transaction.objectStore(db_store_name);
        let key = system_name + '_' + snapshot_index;
        let get_request = file.get(key);
        get_request.onsuccess = () => {
            if (get_request.result !== undefined) {
                let num_bytes = get_request.result.length;
                console.log('fs_js_load_snapshot:', key, 'successfully loaded', num_bytes, 'bytes');
                let ptr = _fs_emsc_alloc(num_bytes);
                HEAPU8.set(get_request.result, ptr);
                _fs_emsc_load_snapshot_callback(context, ptr, num_bytes);
            } else {
                _fs_emsc_load_snapshot_callback(context, 0, 0);
            }
        };
        get_request.onerror = () => {
            console.log('fs_js_load_snapshot: FAILED loading', key);
        };
        transaction.onerror = () => {
            console.log('fs_js_load_snapshot: transaction onerror');
        };
    };
    open_request.onerror = () => {
        console.log('fs_js_load_snapshot: open_request onerror');
    }
});

bool fs_save_snapshot(const char* system_name, size_t snapshot_index, chips_range_t data) {
    assert(system_name && data.ptr && data.size > 0);
    fs_js_save_snapshot(system_name, (int)snapshot_index, data.ptr, data.size);
    return true;
}

EMSCRIPTEN_KEEPALIVE void* fs_emsc_alloc(int size) {
    return malloc((size_t)size);
}

EMSCRIPTEN_KEEPALIVE void fs_emsc_load_snapshot_callback(const fs_snapshot_load_context_t* ctx, void* bytes, int num_bytes) {
    size_t snapshot_index = ctx->snapshot_index;
    fs_snapshot_load_callback_t callback = ctx->callback;
    if (bytes) {
        callback(&(fs_snapshot_response_t){
            .snapshot_index = snapshot_index,
            .result = FS_RESULT_SUCCESS,
            .data = {
                .ptr = bytes,
                .size = (size_t)num_bytes
            }
        });
        free(bytes);
    }
    else {
        callback(&(fs_snapshot_response_t){
            .snapshot_index = snapshot_index,
            .result = FS_RESULT_FAILED,
        });
    }
    free((void*)ctx);
}

bool fs_start_load_snapshot(size_t slot_index, const char* system_name, size_t snapshot_index, fs_snapshot_load_callback_t callback) {
    assert(slot_index < FS_NUM_SLOTS);
    assert(system_name && callback);
    (void)slot_index;

    // allocate a 'context' struct which needs to be tunneled through JS to the fs_emsc_load_snapshot_callback() function
    fs_snapshot_load_context_t* context = calloc(1, sizeof(fs_snapshot_load_context_t));
    context->snapshot_index = snapshot_index;
    context->callback = callback;

    fs_js_load_snapshot(system_name, (int)snapshot_index, context);

    return true;
}
#else // Apple or Linux

bool fs_save_snapshot(const char* system_name, size_t snapshot_index, chips_range_t data) {
    assert(system_name && data.ptr && data.size > 0);
    fs_path_t path = fs_make_snapshot_path("/tmp", system_name, snapshot_index);
    if (path.clamped) {
        return false;
    }
    FILE* fp = fopen(path.cstr, "wb");
    if (fp) {
        fwrite(data.ptr, data.size, 1, fp);
        fclose(fp);
        return true;
    }
    else {
        return false;
    }
}

bool fs_start_load_snapshot(size_t slot_index, const char* system_name, size_t snapshot_index, fs_snapshot_load_callback_t callback) {
    assert(slot_index < FS_NUM_SLOTS);
    assert(system_name && callback);
    fs_path_t path = fs_make_snapshot_path("/tmp", system_name, snapshot_index);
    if (path.clamped) {
        return false;
    }
    fs_snapshot_load_context_t context = {
        .snapshot_index = snapshot_index,
        .callback = callback
    };
    fs_slot_t* slot = &state.slots[slot_index];
    sfetch_send(&(sfetch_request_t){
        .path = path.cstr,
        .channel = (int)slot_index,
        .callback = fs_snapshot_fetch_callback,
        .buffer = { .ptr = slot->buf, .size = FS_MAX_SIZE },
        .user_data = { .ptr = &context, .size = sizeof(context) }
    });
    return true;
}
#endif
