#pragma once
#include <stdint.h>
#include <stddef.h>
#include "chips/chips_common.h"

typedef struct {
    void (*enable_external_debugger)(void);
    void (*boot)(void);
    void (*reset)(void);
    bool (*quickload)(chips_range_t data, bool start, bool stop_on_entry);
    void (*dbg_add_breakpoint)(uint16_t addr);
    void (*dbg_remove_breakpoint)(uint16_t addr);
    void (*dbg_break)(void);
    void (*dbg_continue)(void);
    void (*dbg_step_next)(void);
    void (*dbg_step_into)(void);
} webapi_interface_t;

typedef struct {
    webapi_interface_t funcs;
} webapi_desc_t;

void webapi_init(const webapi_desc_t* desc);
// break_type: UI_DBG_BREAKTYPE_EXEC
void webapi_event_stopped(int break_type, uint16_t addr);
void webapi_event_continued(void);