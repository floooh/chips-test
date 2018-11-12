//------------------------------------------------------------------------------
//  z80dasm-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "util/z80dasm.h"
#include "test.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const uint8_t* base;
    uint16_t pc;
    uint16_t end_pc;
    size_t str_pos;
    char str[32];
} ctx_t;
ctx_t ctx;

static void init(uint16_t pc, const uint8_t* base, size_t len) {
    ctx.base = base;
    ctx.pc = pc;
    ctx.end_pc = pc + len;
    ctx.str_pos = 0;
    memset(ctx.str, 0, sizeof(ctx.str));
}

static uint8_t in_cb(void* user_data) {
    if (ctx.pc < ctx.end_pc) {
        return ctx.base[ctx.pc++];
    }
    else {
        return 0;
    }
}

static void out_cb(char c, void* user_data) {
    if (ctx.str_pos < (sizeof(ctx.str)+1)) {
        ctx.str[ctx.str_pos++] = c;
        ctx.str[ctx.str_pos] = 0;
    }
}

static bool op(const char* res) {
    ctx.str_pos = 0;
    ctx.pc = z80dasm_op(ctx.pc, in_cb, out_cb, 0);
    return 0 == strcmp(ctx.str, res);
}

/* LD r,s; LD r,n */
void LD_r_sn(void) {
    test("LD r,s; LD r,n");
    uint8_t prog[] = {
        0x3E, 0x12,     // LD A,0x12
        0x47,           // LD B,A
        0x4F,           // LD C,A
        0x57,           // LD D,A
        0x5F,           // LD E,A
        0x67,           // LD H,A
        0x6F,           // LD L,A
        0x7F,           // LD A,A

        0x06, 0x13,     // LD B,0x13
        0x40,           // LD B,B
        0x48,           // LD C,B
        0x50,           // LD D,B
        0x58,           // LD E,B
        0x60,           // LD H,B
        0x68,           // LD L,B
        0x78,           // LD A,B

        0x0E, 0x14,     // LD C,0x14
        0x41,           // LD B,C
        0x49,           // LD C,C
        0x51,           // LD D,C
        0x59,           // LD E,C
        0x61,           // LD H,C
        0x69,           // LD L,C
        0x79,           // LD A,C

        0x16, 0x15,     // LD D,0x15
        0x42,           // LD B,D
        0x4A,           // LD C,D
        0x52,           // LD D,D
        0x5A,           // LD E,D
        0x62,           // LD H,D
        0x6A,           // LD L,D
        0x7A,           // LD A,D

        0x1E, 0x16,     // LD E,0x16
        0x43,           // LD B,E
        0x4B,           // LD C,E
        0x53,           // LD D,E
        0x5B,           // LD E,E
        0x63,           // LD H,E
        0x6B,           // LD L,E
        0x7B,           // LD A,E

        0x26, 0x17,     // LD H,0x17
        0x44,           // LD B,H
        0x4C,           // LD C,H
        0x54,           // LD D,H
        0x5C,           // LD E,H
        0x64,           // LD H,H
        0x6C,           // LD L,H
        0x7C,           // LD A,H

        0x2E, 0x18,     // LD L,0x18
        0x45,           // LD B,L
        0x4D,           // LD C,L
        0x55,           // LD D,L
        0x5D,           // LD E,L
        0x65,           // LD H,L
        0x6D,           // LD L,L
        0x7D,           // LD A,L
    };
    init(0, prog, sizeof(prog));
    T(op("LD A,12H"));
    T(op("LD B,A"));
    T(op("LD C,A"));
    T(op("LD D,A"));
    T(op("LD E,A"));
    T(op("LD H,A"));
    T(op("LD L,A"));
    T(op("LD A,A"));
    T(op("LD B,13H"));
    T(op("LD B,B"));
    T(op("LD C,B"));
    T(op("LD D,B"));
    T(op("LD E,B"));
    T(op("LD H,B"));
    T(op("LD L,B"));
    T(op("LD A,B"));
    T(op("LD C,14H"));
    T(op("LD B,C"));
    T(op("LD C,C"));
    T(op("LD D,C"));
    T(op("LD E,C"));
    T(op("LD H,C"));
    T(op("LD L,C"));
    T(op("LD A,C"));
    T(op("LD D,15H"));
    T(op("LD B,D"));
    T(op("LD C,D"));
    T(op("LD D,D"));
    T(op("LD E,D"));
    T(op("LD H,D"));
    T(op("LD L,D"));
    T(op("LD A,D"));
    T(op("LD E,16H"));
    T(op("LD B,E"));
    T(op("LD C,E"));
    T(op("LD D,E"));
    T(op("LD E,E"));
    T(op("LD H,E"));
    T(op("LD L,E"));
    T(op("LD A,E"));
    T(op("LD H,17H"));
    T(op("LD B,H"));
    T(op("LD C,H"));
    T(op("LD D,H"));
    T(op("LD E,H"));
    T(op("LD H,H"));
    T(op("LD L,H"));
    T(op("LD A,H"));
    T(op("LD L,18H"));
    T(op("LD B,L"));
    T(op("LD C,L"));
    T(op("LD D,L"));
    T(op("LD E,L"));
    T(op("LD H,L"));
    T(op("LD L,L"));
    T(op("LD A,L"));
}

/* LD r,(HL) */
void LD_r_iHLi(void) {
    test("LD r,(HL)");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,   // LD HL,0x1000
        0x3E, 0x33,         // LD A,0x33
        0x77,               // LD (HL),A
        0x3E, 0x22,         // LD A,0x22
        0x46,               // LD B,(HL)
        0x4E,               // LD C,(HL)
        0x56,               // LD D,(HL)
        0x5E,               // LD E,(HL)
        0x66,               // LD H,(HL)
        0x26, 0x10,         // LD H,0x10
        0x6E,               // LD L,(HL)
        0x2E, 0x00,         // LD L,0x00
        0x7E,               // LD A,(HL)
    };
    init(0, prog, sizeof(prog));
    T(op("LD HL,1000H"));
    T(op("LD A,33H"));
    T(op("LD (HL),A"));
    T(op("LD A,22H"));
    T(op("LD B,(HL)"));
    T(op("LD C,(HL)"));
    T(op("LD D,(HL)"));
    T(op("LD E,(HL)"));
    T(op("LD H,(HL)"));
    T(op("LD H,10H"));
    T(op("LD L,(HL)"));
    T(op("LD L,00H"));
    T(op("LD A,(HL)"));
}

/* LD r,(IX|IY+d) */
void LD_r_iIXIYi(void) {
    test("LD r,(IX+d); LD r,(IY+d)");
    uint8_t prog[] = {
        0xDD, 0x21, 0x03, 0x10,     // LD IX,0x1003
        0x3E, 0x12,                 // LD A,0x12
        0xDD, 0x77, 0x00,           // LD (IX+0),A
        0x06, 0x13,                 // LD B,0x13
        0xDD, 0x70, 0x01,           // LD (IX+1),B
        0x0E, 0x14,                 // LD C,0x14
        0xDD, 0x71, 0x02,           // LD (IX+2),C
        0x16, 0x15,                 // LD D,0x15
        0xDD, 0x72, 0xFF,           // LD (IX-1),D
        0x1E, 0x16,                 // LD E,0x16
        0xDD, 0x73, 0xFE,           // LD (IX-2),E
        0x26, 0x17,                 // LD H,0x17
        0xDD, 0x74, 0x03,           // LD (IX+3),H
        0x2E, 0x18,                 // LD L,0x18
        0xDD, 0x75, 0xFD,           // LD (IX-3),L
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0x3E, 0x12,                 // LD A,0x12
        0xFD, 0x77, 0x00,           // LD (IY+0),A
        0x06, 0x13,                 // LD B,0x13
        0xFD, 0x70, 0x01,           // LD (IY+1),B
        0x0E, 0x14,                 // LD C,0x14
        0xFD, 0x71, 0x02,           // LD (IY+2),C
        0x16, 0x15,                 // LD D,0x15
        0xFD, 0x72, 0xFF,           // LD (IY-1),D
        0x1E, 0x16,                 // LD E,0x16
        0xFD, 0x73, 0xFE,           // LD (IY-2),E
        0x26, 0x17,                 // LD H,0x17
        0xFD, 0x74, 0x03,           // LD (IY+3),H
        0x2E, 0x18,                 // LD L,0x18
        0xFD, 0x75, 0xFD,           // LD (IY-3),L
    };
    init(0, prog, sizeof(prog));
    T(op("LD IX,1003H"));
    T(op("LD A,12H"));
    T(op("LD (IX+0),A"));
    T(op("LD B,13H"));
    T(op("LD (IX+1),B"));
    T(op("LD C,14H"));
    T(op("LD (IX+2),C"));
    T(op("LD D,15H"));
    T(op("LD (IX-1),D"));
    T(op("LD E,16H"));
    T(op("LD (IX-2),E"));
    T(op("LD H,17H"));
    T(op("LD (IX+3),H"));
    T(op("LD L,18H"));
    T(op("LD (IX-3),L"));
    T(op("LD IY,1003H"));
    T(op("LD A,12H"));
    T(op("LD (IY+0),A"));
    T(op("LD B,13H"));
    T(op("LD (IY+1),B"));
    T(op("LD C,14H"));
    T(op("LD (IY+2),C"));
    T(op("LD D,15H"));
    T(op("LD (IY-1),D"));
    T(op("LD E,16H"));
    T(op("LD (IY-2),E"));
    T(op("LD H,17H"));
    T(op("LD (IY+3),H"));
    T(op("LD L,18H"));
    T(op("LD (IY-3),L"));
}

int main() {
    test_begin("z80dasm-test");
    LD_r_sn();
    LD_r_iHLi();
    LD_r_iIXIYi();
    return test_end();
}

