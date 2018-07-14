#pragma once
/*
    Simple file access functions.
*/
extern void fs_init(void);
extern bool fs_load_file(const char* path);
extern void fs_load_mem(const uint8_t* ptr, uint32_t size);
extern uint32_t fs_size(void);
extern const uint8_t* fs_ptr(void);
extern void fs_free(void);

