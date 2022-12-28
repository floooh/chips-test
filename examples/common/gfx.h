#pragma once
/*
    Common graphics functions for the chips-test example emulators.

    REMINDER: consider using this CRT shader?

    https://github.com/mattiasgustavsson/rebasic/blob/master/source/libs/crtemu.h
*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "chips/chips_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int top, bottom, left, right;
} gfx_border_t;

typedef struct {
    gfx_border_t border;
    chips_display_info_t display_info;
    chips_dim_t pixel_aspect;   // optional pixel aspect ratio, default is 1:1
    void (*draw_extra_cb)(void);
} gfx_desc_t;

void gfx_init(const gfx_desc_t* desc);
void gfx_draw(chips_display_info_t display_info);
void gfx_shutdown(void);

// UI helper functions unrelated to actual emulator framebuffer rendering
void* gfx_create_texture(int w, int h);
void gfx_update_texture(void* h, void* data, int data_byte_size);
void gfx_destroy_texture(void* h);
void* gfx_create_screenshot_texture(chips_display_info_t display_info);
void* gfx_shared_empty_snapshot_texture(void);
void gfx_flash_success(void);
void gfx_flash_error(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
