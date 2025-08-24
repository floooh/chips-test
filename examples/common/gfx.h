#pragma once
/*
    Common graphics functions for the chips-test example emulators.

    REMINDER: consider using this CRT shader?

    https://github.com/mattiasgustavsson/rebasic/blob/master/source/libs/crtemu.h
*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "sokol_gfx.h"
#include "chips/chips_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int top, bottom, left, right;
} gfx_border_t;

typedef struct {
    sg_view display_texview;
    sg_sampler display_sampler;
    chips_display_info_t display_info;
} gfx_draw_info_t;

typedef void(*gfx_init_extra_t)(void);
typedef void(*gfx_draw_extra_t)(const gfx_draw_info_t* draw_info);

typedef struct {
    bool disable_speaker_icon;
    gfx_border_t border;
    chips_display_info_t display_info;
    chips_dim_t pixel_aspect;   // optional pixel aspect ratio, default is 1:1
    gfx_init_extra_t init_extra_cb;
    gfx_draw_extra_t draw_extra_cb;
} gfx_desc_t;

void gfx_init(const gfx_desc_t* desc);
void gfx_draw(chips_display_info_t display_info);
void gfx_shutdown(void);
void gfx_flash_success(void);
void gfx_flash_error(void);
void gfx_disable_speaker_icon(void);
chips_dim_t gfx_pixel_aspect(void);
sg_view gfx_create_icon_texview(const uint8_t* packed_pixels, int width, int height, int stride, const char* label);

#ifdef __cplusplus
} /* extern "C" */
#endif
