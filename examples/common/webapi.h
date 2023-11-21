#pragma once
#include <stdint.h>
#include <stddef.h>
#include "chips/chips_common.h"

#define WEBAPI_STOPREASON_UNKNOWN       (0)
#define WEBAPI_STOPREASON_BREAK         (1)
#define WEBAPI_STOPREASON_BREAKPOINT    (2)
#define WEBAPI_STOPREASON_STEP          (3)
#define WEBAPI_STOPREASON_ENTRY         (4)
#define WEBAPI_STOPREASON_EXIT          (5)

#define WEBAPI_CPUTYPE_INVALID (0)
#define WEBAPI_CPUTYPE_Z80     (1)
#define WEBAPI_CPUTYPE_6502    (2)

#define WEBAPI_CPUSTATE_TYPE (0)
#define WEBAPI_CPUSTATE_AF   (1)
#define WEBAPI_CPUSTATE_BC   (2)
#define WEBAPI_CPUSTATE_DE   (3)
#define WEBAPI_CPUSTATE_HL   (4)
#define WEBAPI_CPUSTATE_IX   (5)
#define WEBAPI_CPUSTATE_IY   (6)
#define WEBAPI_CPUSTATE_SP   (7)
#define WEBAPI_CPUSTATE_PC   (8)
#define WEBAPI_CPUSTATE_AF2  (9)
#define WEBAPI_CPUSTATE_BC2  (10)
#define WEBAPI_CPUSTATE_DE2  (11)
#define WEBAPI_CPUSTATE_HL2  (12)
#define WEBAPI_CPUSTATE_IM   (13)
#define WEBAPI_CPUSTATE_IR   (14)
#define WEBAPI_CPUSTATE_IFF  (15)   // bit0: iff1, bit2: iff2

#define WEBAPI_CPUSTATE_MAX  (16)

typedef struct webapi_cpu_state_t {
    uint16_t items[WEBAPI_CPUSTATE_MAX];
} webapi_cpu_state_t;

typedef struct {
    void (*boot)(void);
    void (*reset)(void);
    bool (*ready)(void);
    bool (*quickload)(chips_range_t data, bool start, bool stop_on_entry);
    void (*dbg_connect)(void);
    void (*dbg_disconnect)(void);
    void (*dbg_add_breakpoint)(uint16_t addr);
    void (*dbg_remove_breakpoint)(uint16_t addr);
    void (*dbg_break)(void);
    void (*dbg_continue)(void);
    void (*dbg_step_next)(void);
    void (*dbg_step_into)(void);
    webapi_cpu_state_t (*dbg_cpu_state)(void);
} webapi_interface_t;

typedef struct {
    webapi_interface_t funcs;
} webapi_desc_t;

void webapi_init(const webapi_desc_t* desc);
// stop_reason: WEBAPI_STOPREASON_xxx
void webapi_event_stopped(int stop_reason, uint16_t addr);
void webapi_event_continued(void);