#pragma once
/*
    Common graphics functions for the chips-test example emulators.

    REMINDER: consider using this CRT shader?

    https://github.com/mattiasgustavsson/rebasic/blob/master/source/libs/crtemu.h
*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GFX_MAX_FB_WIDTH (1024)
#define GFX_MAX_FB_HEIGHT (1024)

typedef struct {
    int top, bottom, left, right;
} gfx_border_t;

typedef struct {
    int width, height;
} gfx_dim_t;

typedef struct {
    int x, y, width, height;
} gfx_rect_t;

typedef struct {
    const void* ptr;    // up to 256 RGBA8 colors
    size_t size;        // palette size in bytes (4 * num_colors)
} gfx_range_t;

typedef struct {
    gfx_border_t border;
    gfx_dim_t pixel_aspect; // optional pixel aspect ratio, default is 1:1
    gfx_range_t palette;    // optional color palette, up to 256 entries
    bool portrait;          // true if screen is in portrait mode (e.g. most arcade machines)
    void (*draw_extra_cb)(void);
} gfx_desc_t;

typedef struct {
    gfx_dim_t fb;           // the framebuffer width and height
    gfx_rect_t view;        // the visible screen rectangle inside the framebuffer
} gfx_draw_t;

void gfx_init(const gfx_desc_t* desc);
void* gfx_framebuffer_ptr(void);
size_t gfx_framebuffer_size(void);
void gfx_draw(const gfx_draw_t* draw_args);
void gfx_shutdown(void);

// UI helper functions unrelated to actual emulator framebuffer rendering
void* gfx_create_texture(gfx_dim_t size);
void gfx_update_texture(void* h, void* data, int data_byte_size);
void gfx_destroy_texture(void* h);
void gfx_flash_success(void);
void gfx_flash_error(void);

#ifdef __cplusplus
} /* extern "C" */
#endif