#pragma once
#include <stdint.h>

void fs_init(void);
void fs_dowork(void);
void fs_start_load_file(const char* path);
void fs_start_load_dropped_file(void);
bool fs_load_base64(const char* name, const char* payload);
void fs_load_mem(const char* path, const uint8_t* ptr, uint32_t size);
uint32_t fs_size(void);
const uint8_t* fs_ptr(void);
void fs_reset(void);
bool fs_ext(const char* str);
const char* fs_filename(void);
