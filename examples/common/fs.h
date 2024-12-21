#pragma once
#include <stdint.h>
#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

// standard loading slots
typedef enum {
    FS_CHANNEL_IMAGES = 0,
    FS_CHANNEL_SNAPSHOTS,
    //---
    FS_CHANNEL_NUM,
} fs_channel_t;

typedef enum {
    FS_RESULT_IDLE,
    FS_RESULT_FAILED,
    FS_RESULT_PENDING,
    FS_RESULT_SUCCESS,
} fs_result_t;

typedef struct {
    size_t snapshot_index;
    fs_result_t result;
    chips_range_t data;
} fs_snapshot_response_t;

typedef void (*fs_snapshot_load_callback_t)(const fs_snapshot_response_t* response);

void fs_init(void);
void fs_dowork(void);
void fs_reset(fs_channel_t chn);
void fs_load_file_async(fs_channel_t chn, const char* path);
void fs_load_dropped_file_async(fs_channel_t chn);
bool fs_load_base64(fs_channel_t chn, const char* name, const char* payload);
bool fs_save_snapshot(const char* system_name, size_t snapshot_index, chips_range_t data);
bool fs_load_snapshot_async(const char* system_name, size_t snapshot_index, fs_snapshot_load_callback_t callback);
fs_result_t fs_result(fs_channel_t chn);
bool fs_success(fs_channel_t chn);
bool fs_failed(fs_channel_t chn);
bool fs_pending(fs_channel_t chn);
chips_range_t fs_data(fs_channel_t chn);
bool fs_ext(fs_channel_t chn, const char* str);
void fs_save_ini(const char* key, const char* payload);
const char* fs_load_ini(const char* key);
void fs_free_ini(const char* payload_or_null);

#if defined(__cplusplus)
} // extern "C"
#endif
