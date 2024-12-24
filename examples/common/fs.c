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
#include <stdarg.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#if defined(WIN32)
#include <windows.h>
#endif

#define FS_EXT_SIZE (16)
#define FS_PATH_SIZE (2048)
#define FS_MAX_SIZE (2024 * 1024)

typedef struct {
    char cstr[FS_PATH_SIZE];
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
} fs_channel_state_t;

typedef struct {
    bool valid;
    fs_channel_state_t channels[FS_CHANNEL_NUM];
} fs_state_t;
static fs_state_t state;

void fs_init(void) {
    memset(&state, 0, sizeof(state));
    state.valid = true;
    sfetch_setup(&(sfetch_desc_t){
        .max_requests = 128,
        .num_channels = FS_CHANNEL_NUM,
        .num_lanes = 1,
        .logger.func = slog_func,
    });
}

void fs_dowork(void) {
    assert(state.valid);
    sfetch_dowork();
}

static void fs_path_reset(fs_path_t* path) {
    memset(path->cstr, 0, sizeof(path->cstr));
    path->clamped = false;
}

#if defined (WIN32)
bool fs_win32_path_to_wide(const fs_path_t* path, WCHAR* out_buf, size_t out_buf_size_in_bytes) {
    if ((path->cstr[0] == 0) || (path->clamped)) {
        return false;
    }
    int out_num_chars = (int)(out_buf_size_in_bytes / sizeof(wchar_t));
    if (0 == MultiByteToWideChar(CP_UTF8, 0, path->cstr, -1, out_buf, out_num_chars)) {
        return false;
    }
    return true;
}
#endif

#if defined(__GNUC__)
static fs_path_t fs_path_printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
#endif
static fs_path_t fs_path_printf(const char* fmt, ...) {
    fs_path_t path = {0};
    va_list args;
    va_start(args, fmt);
    int res = vsnprintf(path.cstr, sizeof(path.cstr), fmt, args);
    va_end(args);
    path.clamped = res >= (int)sizeof(path.cstr);
    return path;
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

fs_result_t fs_result(fs_channel_t chn) {
    assert(state.valid);
    assert(chn < FS_CHANNEL_NUM);
    return state.channels[chn].result;
}

bool fs_success(fs_channel_t chn) {
    return fs_result(chn) == FS_RESULT_SUCCESS;
}

bool fs_failed(fs_channel_t chn) {
    return fs_result(chn) == FS_RESULT_FAILED;
}

bool fs_pending(fs_channel_t chn) {
    return fs_result(chn) == FS_RESULT_PENDING;
}

chips_range_t fs_data(fs_channel_t chn) {
    assert(state.valid);
    assert(chn < FS_CHANNEL_NUM);
    fs_channel_state_t* channel = &state.channels[chn];
    if (channel->result == FS_RESULT_SUCCESS) {
        return (chips_range_t){ .ptr = channel->ptr, .size = channel->size };
    }
    else {
        return (chips_range_t){0};
    }
}

// http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
static const unsigned char fs_base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static bool fs_base64_decode(fs_channel_state_t* channel, const char* src) {
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
    if (olen >= (int)sizeof(channel->buf)) {
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
            channel->buf[channel->size++] = (block[0] << 2) | (block[1] >> 4);
            channel->buf[channel->size++] = (block[1] << 4) | (block[2] >> 2);
            channel->buf[channel->size++] = (block[2] << 6) | block[3];
            if (pad > 0) {
                if (pad <= 2) {
                    channel->size -= pad;
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

bool fs_ext(fs_channel_t chn, const char* ext) {
    assert(state.valid);
    assert(chn < FS_CHANNEL_NUM);
    char buf[FS_EXT_SIZE];
    fs_path_extract_extension(&state.channels[chn].path, buf, sizeof(buf));
    return 0 == strcmp(ext, buf);
}

const char* fs_filename(fs_channel_t chn) {
    assert(state.valid);
    assert(chn < FS_CHANNEL_NUM);
    return state.channels[chn].path.cstr;
}

void fs_reset(fs_channel_t chn) {
    assert(state.valid);
    assert(chn < FS_CHANNEL_NUM);
    fs_channel_state_t* channel = &state.channels[chn];
    fs_path_reset(&channel->path);
    channel->result = FS_RESULT_IDLE;
    channel->ptr = 0;
    channel->size = 0;
}

bool fs_load_base64(fs_channel_t chn, const char* name, const char* payload) {
    assert(state.valid);
    assert(chn < FS_CHANNEL_NUM);
    fs_reset(chn);
    fs_channel_state_t* channel = &state.channels[chn];
    channel->path = fs_path_printf("%s", name);
    if (fs_base64_decode(channel, payload)) {
        channel->result = FS_RESULT_SUCCESS;
        channel->ptr = channel->buf;
        return true;
    }
    else {
        channel->result = FS_RESULT_FAILED;
        return false;
    }
}

static void fs_fetch_callback(const sfetch_response_t* response) {
    assert(state.valid);
    fs_channel_t chn = *(fs_channel_t*)response->user_data;
    assert(chn < FS_CHANNEL_NUM);
    fs_channel_state_t* channel = &state.channels[chn];
    if (response->fetched) {
        channel->result = FS_RESULT_SUCCESS;
        channel->ptr = (uint8_t*)response->data.ptr;
        channel->size = response->data.size;
        assert(channel->size < sizeof(channel->buf));
        // in case it's a text file, zero-terminate the data
        channel->buf[channel->size] = 0;
    }
    else if (response->failed) {
        channel->result = FS_RESULT_FAILED;
    }
}

#if defined(__EMSCRIPTEN__)

EM_JS_DEPS(chips_ini, "$UTF8ToString,$stringToNewUTF8");

EM_JS(void, emsc_js_save_ini, (const char* c_key, const char* c_payload), {
    if (window.localStorage === undefined) {
        return;
    }
    const key = UTF8ToString(c_key);
    const payload = UTF8ToString(c_payload);
    window.localStorage.setItem(key, payload);
});

EM_JS(const char*, emsc_js_load_ini, (const char* c_key), {
    if (window.localStorage === undefined) {
        return 0;
    }
    const key = UTF8ToString(c_key);
    const payload = window.localStorage.getItem(key);
    if (payload) {
        return stringToNewUTF8(payload);
    } else {
        return 0;
    }
});

static void fs_emsc_dropped_file_callback(const sapp_html5_fetch_response* response) {
    fs_channel_t chn = (fs_channel_t)(uintptr_t)response->user_data;
    assert(chn < FS_CHANNEL_NUM);
    fs_channel_state_t* channel = &state.channels[chn];
    if (response->succeeded) {
        channel->result = FS_RESULT_SUCCESS;
        channel->ptr = (uint8_t*)response->data.ptr;
        channel->size = response->data.size;
        assert(channel->size < sizeof(channel->buf));
        // in case it's a text file, zero-terminate the data
        channel->buf[channel->size] = 0;
    }
    else {
        channel->result = FS_RESULT_FAILED;
    }
}

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
        const db = open_request.result;
        db.createObjectStore(db_store_name);
    };
    open_request.onsuccess = () => {
        console.log('fs_js_save_snapshot: onsuccess');
        const db = open_request.result;
        const transaction = db.transaction([db_store_name], 'readwrite');
        const file = transaction.objectStore(db_store_name);
        const key = system_name + '_' + snapshot_index;
        const blob = HEAPU8.subarray(bytes, bytes + num_bytes);
        const put_request = file.put(blob, key);
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
        const db = open_request.result;
        db.createObjectStore(db_store_name);
    };
    open_request.onsuccess = () => {
        const db = open_request.result;
        let transaction;
        try {
            transaction = db.transaction([db_store_name], 'readwrite');
        } catch (e) {
            console.log('fs_js_load_snapshot: db.transaction failed with', e);
            return;
        };
        const file = transaction.objectStore(db_store_name);
        const key = system_name + '_' + snapshot_index;
        const get_request = file.get(key);
        get_request.onsuccess = () => {
            if (get_request.result !== undefined) {
                const num_bytes = get_request.result.length;
                console.log('fs_js_load_snapshot:', key, 'successfully loaded', num_bytes, 'bytes');
                const ptr = _fs_emsc_alloc(num_bytes);
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

bool fs_emsc_save_snapshot(const char* system_name, size_t snapshot_index, chips_range_t data) {
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

bool fs_emsc_load_snapshot_async(const char* system_name, size_t snapshot_index, fs_snapshot_load_callback_t callback) {
    assert(system_name && callback);

    // allocate a 'context' struct which needs to be tunneled through JS to the fs_emsc_load_snapshot_callback() function
    fs_snapshot_load_context_t* context = calloc(1, sizeof(fs_snapshot_load_context_t));
    context->snapshot_index = snapshot_index;
    context->callback = callback;

    fs_js_load_snapshot(system_name, (int)snapshot_index, context);

    return true;
}
#else // any native platform
static void fs_win32_posix_snapshot_fetch_callback(const sfetch_response_t* response) {
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

static bool fs_win32_posix_write_file(fs_path_t path, chips_range_t data) {
    if (path.clamped) {
        return false;
    }
    #if defined(WIN32)
        WCHAR wc_path[FS_PATH_SIZE];
        if (!fs_win32_path_to_wide(&path, wc_path, sizeof(wc_path))) {
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
    #else
        FILE* fp = fopen(path.cstr, "wb");
        if (!fp) {
            return false;
        }
        fwrite(data.ptr, data.size, 1, fp);
        fclose(fp);
    #endif
    return true;
}

// NOTE: free the returned range.ptr with free(ptr)
static chips_range_t fs_win32_posix_read_file(fs_path_t path, bool null_terminated) {
    if (path.clamped) {
        return (chips_range_t){0};
    }
    #if defined(WIN32)
        WCHAR wc_path[FS_PATH_SIZE];
        if (!fs_win32_path_to_wide(&path, wc_path, sizeof(wc_path))) {
            return (chips_range_t){0};
        }
        HANDLE fp = CreateFileW(wc_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fp == INVALID_HANDLE_VALUE) {
            return (chips_range_t){0};
        }
        size_t file_size = GetFileSize(fp, NULL);
        size_t alloc_size = null_terminated ? file_size + 1 : file_size;
        void* ptr = calloc(1, alloc_size);
        DWORD read_bytes = 0;
        BOOL read_res = ReadFile(fp, ptr, file_size, &read_bytes, NULL);
        CloseHandle(fp);
        if (read_res && read_bytes == file_size) {
            return (chips_range_t){ .ptr = ptr, .size = alloc_size };
        } else {
            free(ptr);
            return (chips_range_t){0};
        }
    #else
        FILE* fp = fopen(path.cstr, "rb");
        if (!fp) {
            return (chips_range_t){0};
        }
        fseek(fp, 0, SEEK_END);
        size_t file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        size_t alloc_size = null_terminated ? file_size + 1 : file_size;
        void* ptr = calloc(1, alloc_size);
        size_t bytes_read = fread(ptr, 1, file_size, fp);
        fclose(fp);
        if (bytes_read == file_size) {
            return (chips_range_t){ .ptr = ptr, .size = alloc_size };
        } else {
            free(ptr);
            return (chips_range_t){0};
        }
    #endif
}

static fs_path_t fs_win32_posix_tmp_dir(void) {
    #if defined(WIN32)
    WCHAR wc_tmp_path[FS_PATH_SIZE];
    if (0 == GetTempPathW(sizeof(wc_tmp_path) / sizeof(WCHAR), wc_tmp_path)) {
        return (fs_path_t){0};
    }
    char utf8_tmp_path[FS_PATH_SIZE];
    if (0 == WideCharToMultiByte(CP_UTF8, 0, wc_tmp_path, -1, utf8_tmp_path, sizeof(utf8_tmp_path), NULL, NULL)) {
        return (fs_path_t){0};
    }
    return fs_path_printf("%s", utf8_tmp_path);
    #else
    return fs_path_printf("%s", "/tmp");
    #endif
}

fs_path_t fs_win32_posix_make_snapshot_path(const char* system_name, size_t snapshot_index) {
    fs_path_t tmp_dir = fs_win32_posix_tmp_dir();
    return fs_path_printf("%s/chips_%s_snapshot_%zu", tmp_dir.cstr, system_name, snapshot_index);
}

fs_path_t fs_win32_posix_make_ini_path(const char* key) {
    fs_path_t tmp_dir = fs_win32_posix_tmp_dir();
    return fs_path_printf("%s/%s_imgui.ini", tmp_dir.cstr, key);
}

bool fs_win32_posix_save_snapshot(const char* system_name, size_t snapshot_index, chips_range_t data) {
    fs_path_t path = fs_win32_posix_make_snapshot_path(system_name, snapshot_index);
    return fs_win32_posix_write_file(path, data);
}

bool fs_win32_posix_load_snapshot_async(const char* system_name, size_t snapshot_index, fs_snapshot_load_callback_t callback) {
    assert(system_name && callback);
    fs_path_t path = fs_win32_posix_make_snapshot_path(system_name, snapshot_index);
    if (path.clamped) {
        return false;
    }
    fs_snapshot_load_context_t context = {
        .snapshot_index = snapshot_index,
        .callback = callback
    };
    const fs_channel_t chn = FS_CHANNEL_SNAPSHOTS;
    fs_channel_state_t* channel = &state.channels[chn];
    sfetch_send(&(sfetch_request_t){
        .path = path.cstr,
        .channel = (int)chn,
        .callback = fs_win32_posix_snapshot_fetch_callback,
        .buffer = { .ptr = channel->buf, .size = FS_MAX_SIZE },
        .user_data = { .ptr = &context, .size = sizeof(context) }
    });
    return true;
}
#endif

void fs_load_file_async(fs_channel_t chn, const char* path) {
    assert(state.valid);
    assert(chn < FS_CHANNEL_NUM);
    fs_reset(chn);
    fs_channel_state_t* channel = &state.channels[chn];
    channel->path = fs_path_printf("%s", path);
    channel->result = FS_RESULT_PENDING;
    sfetch_send(&(sfetch_request_t){
        .path = path,
        .channel = chn,
        .callback = fs_fetch_callback,
        .buffer = { .ptr = channel->buf, .size = FS_MAX_SIZE },
        .user_data = { .ptr = &chn, .size = sizeof(chn) },
    });
}

void fs_load_dropped_file_async(fs_channel_t chn) {
    assert(state.valid);
    assert(chn < FS_CHANNEL_NUM);
    fs_reset(chn);
    fs_channel_state_t* channel = &state.channels[chn];
    const char* path = sapp_get_dropped_file_path(0);
    channel->path = fs_path_printf("%s", path);
    channel->result = FS_RESULT_PENDING;
    #if defined(__EMSCRIPTEN__)
        sapp_html5_fetch_dropped_file(&(sapp_html5_fetch_request){
            .dropped_file_index = 0,
            .callback = fs_emsc_dropped_file_callback,
            .buffer = { .ptr = channel->buf, .size = FS_MAX_SIZE },
            .user_data = (void*)(intptr_t)chn,
        });
    #else
        fs_load_file_async(chn, path);
    #endif
}

bool fs_save_snapshot(const char* system_name, size_t snapshot_index, chips_range_t data) {
    #if defined(__EMSCRIPTEN__)
    return fs_emsc_save_snapshot(system_name, snapshot_index, data);
    #else
    return fs_win32_posix_save_snapshot(system_name, snapshot_index, data);
    #endif
}

bool fs_load_snapshot_async(const char* system_name, size_t snapshot_index, fs_snapshot_load_callback_t callback) {
    #if defined(__EMSCRIPTEN__)
    return fs_emsc_load_snapshot_async(system_name, snapshot_index, callback);
    #else
    return fs_win32_posix_load_snapshot_async(system_name, snapshot_index, callback);
    #endif
}

void fs_save_ini(const char* key, const char* payload) {
    assert(key && payload);
    #if defined(__EMSCRIPTEN__)
    emsc_js_save_ini(key, payload);
    #else
    fs_path_t path = fs_win32_posix_make_ini_path(key);
    chips_range_t data = { .ptr = (void*)payload, .size = strlen(payload) };
    fs_win32_posix_write_file(path, data);
    #endif
}

const char* fs_load_ini(const char* key) {
    assert(key);
    #if defined(__EMSCRIPTEN__)
    return emsc_js_load_ini(key);
    #else
    fs_path_t path = fs_win32_posix_make_ini_path(key);
    chips_range_t data = fs_win32_posix_read_file(path, true);
    return data.ptr;
    #endif
}

void fs_free_ini(const char* payload_or_null) {
    if (payload_or_null) {
        void* payload = (void*)payload_or_null;
        free(payload);
    }
}