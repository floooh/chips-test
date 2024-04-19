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

#define WEBAPI_CPUSTATE_Z80_AF   (1)
#define WEBAPI_CPUSTATE_Z80_BC   (2)
#define WEBAPI_CPUSTATE_Z80_DE   (3)
#define WEBAPI_CPUSTATE_Z80_HL   (4)
#define WEBAPI_CPUSTATE_Z80_IX   (5)
#define WEBAPI_CPUSTATE_Z80_IY   (6)
#define WEBAPI_CPUSTATE_Z80_SP   (7)
#define WEBAPI_CPUSTATE_Z80_PC   (8)
#define WEBAPI_CPUSTATE_Z80_AF2  (9)
#define WEBAPI_CPUSTATE_Z80_BC2  (10)
#define WEBAPI_CPUSTATE_Z80_DE2  (11)
#define WEBAPI_CPUSTATE_Z80_HL2  (12)
#define WEBAPI_CPUSTATE_Z80_IM   (13)
#define WEBAPI_CPUSTATE_Z80_IR   (14)
#define WEBAPI_CPUSTATE_Z80_IFF  (15)   // bit0: iff1, bit2: iff2

#define WEBAPI_CPUSTATE_6502_A  (1)
#define WEBAPI_CPUSTATE_6502_X  (2)
#define WEBAPI_CPUSTATE_6502_Y  (3)
#define WEBAPI_CPUSTATE_6502_S  (4)
#define WEBAPI_CPUSTATE_6502_P  (5)
#define WEBAPI_CPUSTATE_6502_PC (6)

#define WEBAPI_CPUSTATE_MAX  (16)

typedef struct webapi_cpu_state_t {
    uint16_t items[WEBAPI_CPUSTATE_MAX];
} webapi_cpu_state_t;

#define WEBAPI_DASM_LINE_MAX_BYTES (8)
#define WEBAPI_DASM_LINE_MAX_CHARS (32)

typedef struct {
    uint16_t addr;
    uint8_t num_bytes;
    uint8_t num_chars;
    uint8_t bytes[WEBAPI_DASM_LINE_MAX_BYTES];
    char chars[WEBAPI_DASM_LINE_MAX_CHARS];
} webapi_dasm_line_t;

// the webapi_load() function takes a byte blob which is a container
// format for emulator-specific quickload fileformats with an additional
// 16-byte header with meta-information
typedef struct {
    uint8_t magic[4];       // 'CHIP'
    uint8_t type[4];        // 'KCC ', 'PRG ', etc...
    uint8_t start_addr_lo;  // execution starts here
    uint8_t start_addr_hi;
    uint8_t flags;
    uint8_t reserved[5];
    uint8_t payload[];
} webapi_fileheader_t;

#define WEBAPI_FILEHEADER_FLAG_START (1<<0)         // if set, start automatically
#define WEBAPI_FILEHEADER_FLAG_STOPONENTRY (1<<1)   // if set, stop execution on entry

typedef struct {
    void (*boot)(void);
    void (*reset)(void);
    bool (*ready)(void);
    bool (*load)(chips_range_t data);       // data starts with a webapi_fileheader_t
    bool (*load_snapshot)(size_t index);
    void (*save_snapshot)(size_t index);
    void (*dbg_connect)(void);
    void (*dbg_disconnect)(void);
    void (*dbg_add_breakpoint)(uint16_t addr);
    void (*dbg_remove_breakpoint)(uint16_t addr);
    void (*dbg_break)(void);
    void (*dbg_continue)(void);
    void (*dbg_step_next)(void);
    void (*dbg_step_into)(void);
    webapi_cpu_state_t (*dbg_cpu_state)(void);
    void (*dbg_request_disassembly)(uint16_t addr, int offset_lines, int num_lines, webapi_dasm_line_t* dst_lines);
    void (*dbg_read_memory)(uint16_t addr, int num_bytes, uint8_t* dst_ptr);
    void (*input)(char* text);
} webapi_interface_t;

typedef struct {
    webapi_interface_t funcs;
} webapi_desc_t;

void webapi_init(const webapi_desc_t* desc);
// stop_reason: WEBAPI_STOPREASON_xxx
void webapi_event_stopped(int stop_reason, uint16_t addr);
void webapi_event_continued(void);
void webapi_event_reboot(void);
void webapi_event_reset(void);