#pragma once
#include <stdint.h>
#include <stddef.h>

// standard loading slots
#define FS_SLOT_IMAGE (0)
#define FS_SLOT_SNAPSHOTS (1)
#define FS_NUM_SLOTS (2)

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
void fs_reset(size_t slot_index);
void fs_start_load_file(size_t slot_index, const char* path);
void fs_start_load_dropped_file(size_t slot_index);
bool fs_load_base64(size_t slot_index, const char* name, const char* payload);
void fs_load_mem(size_t slot_index, const char* path, chips_range_t data);
bool fs_save_snapshot(const char* system_name, size_t snapshot_index, chips_range_t data);
bool fs_start_load_snapshot(size_t slot_index, const char* system_name, size_t snapshot_index, fs_snapshot_load_callback_t callback);

fs_result_t fs_result(size_t slot_index);
bool fs_success(size_t slot_index);
bool fs_failed(size_t slot_index);
bool fs_pending(size_t slot_index);
chips_range_t fs_data(size_t slot_index);
bool fs_ext(size_t slot_index, const char* str);
