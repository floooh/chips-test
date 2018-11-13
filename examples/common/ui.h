#pragma once
#include "sokol_app.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void ui_init(void);
extern void ui_discard(void);
extern void ui_draw(void);
extern bool ui_input(const sapp_event* event);
#ifdef __cplusplus
} /* extern "C" */
#endif
