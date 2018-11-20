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
    bool *open;      /* optional pointer to 'open flag' */
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

void ui_init(const ui_desc_t* desc);
void ui_set_exec_time(uint64_t t);
void ui_discard(void);
void ui_draw(void);
bool ui_input(const sapp_event* event);
#ifdef __cplusplus
} /* extern "C" */
#endif
