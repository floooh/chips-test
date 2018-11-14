#pragma once
#include "sokol_app.h"

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: all string pointers must point to static strings */
#define UI_MAX_MENU_ITEMS (16)
#define UI_MAX_MENUS (8)

typedef struct {
    const char* name;
    void (*func)(void);
} ui_menuitem_t;

typedef struct {
    const char* name;
    ui_menuitem_t items[UI_MAX_MENU_ITEMS];
} ui_menu_t;

typedef struct {
    void (*draw)(void);
    ui_menu_t menus[UI_MAX_MENUS];
} ui_desc_t;

extern void ui_init(const ui_desc_t* desc);
extern void ui_discard(void);
extern void ui_draw(void);
extern bool ui_input(const sapp_event* event);
#ifdef __cplusplus
} /* extern "C" */
#endif
