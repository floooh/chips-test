#pragma once
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_imgui.h"
#include "chips/chips_common.h"
#include "ui/ui_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t ui_texture_t;

typedef void (*ui_draw_t)(void);
typedef ui_settings_t (*ui_save_settings_t)(void);

typedef struct {
    ui_draw_t draw_cb;
    ui_save_settings_t save_settings_cb;
    const char* imgui_ini_key;
} ui_desc_t;

void ui_init(const ui_desc_t* desc);
const ui_settings_t* ui_settings(void);
void ui_discard(void);
void ui_draw(void);
bool ui_input(const sapp_event* event);
ui_texture_t ui_create_texture(int w, int h);
void ui_update_texture(ui_texture_t h, void* data, int data_byte_size);
void ui_destroy_texture(ui_texture_t h);
ui_texture_t ui_create_screenshot_texture(chips_display_info_t display_info);
ui_texture_t ui_shared_empty_snapshot_texture(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
