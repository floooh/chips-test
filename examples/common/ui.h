#pragma once
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_imgui.h"
#include "chips/chips_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*ui_draw_t)(void);
void ui_init(ui_draw_t draw_cb);
void ui_discard(void);
void ui_draw(void);
bool ui_input(const sapp_event* event);
void* ui_create_texture(int w, int h);
void ui_update_texture(void* h, void* data, int data_byte_size);
void ui_destroy_texture(void* h);
void* ui_create_screenshot_texture(chips_display_info_t display_info);
void* ui_shared_empty_snapshot_texture(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
