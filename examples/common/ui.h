#pragma once
#include "sokol_app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*ui_draw_t)(void);
void ui_init(ui_draw_t draw_cb);
void ui_discard(void);
void ui_draw(void);
bool ui_input(const sapp_event* event);

#ifdef __cplusplus
} /* extern "C" */
#endif
