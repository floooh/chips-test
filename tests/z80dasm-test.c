//------------------------------------------------------------------------------
//  z80dasm-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "util/z80dasm.h"
#include "test.h"
#include <stdio.h>
#include <string.h>

#define TOP(str) T(op(str))

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

/* LD A,R; LD A,I */
void LD_A_RI(void) {
    test("LD A,R; LD A,I");
    uint8_t prog[] = {
        0xED, 0x57,         // LD A,I
        0x97,               // SUB A
        0xED, 0x5F,         // LD A,R
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,I");
    TOP("SUB A");
    TOP("LD A,R");
}

/* LD I,A; LD R,A */
void LD_IR_A(void) {
    test("LD I,A; LD R,A");
    uint8_t prog[] = {
        0x3E, 0x45,     // LD A,0x45
        0xED, 0x47,     // LD I,A
        0xED, 0x4F,     // LD R,A
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,45H");
    TOP("LD I,A");
    TOP("LD R,A");
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
    TOP("LD A,12H");
    TOP("LD B,A");
    TOP("LD C,A");
    TOP("LD D,A");
    TOP("LD E,A");
    TOP("LD H,A");
    TOP("LD L,A");
    TOP("LD A,A");
    TOP("LD B,13H");
    TOP("LD B,B");
    TOP("LD C,B");
    TOP("LD D,B");
    TOP("LD E,B");
    TOP("LD H,B");
    TOP("LD L,B");
    TOP("LD A,B");
    TOP("LD C,14H");
    TOP("LD B,C");
    TOP("LD C,C");
    TOP("LD D,C");
    TOP("LD E,C");
    TOP("LD H,C");
    TOP("LD L,C");
    TOP("LD A,C");
    TOP("LD D,15H");
    TOP("LD B,D");
    TOP("LD C,D");
    TOP("LD D,D");
    TOP("LD E,D");
    TOP("LD H,D");
    TOP("LD L,D");
    TOP("LD A,D");
    TOP("LD E,16H");
    TOP("LD B,E");
    TOP("LD C,E");
    TOP("LD D,E");
    TOP("LD E,E");
    TOP("LD H,E");
    TOP("LD L,E");
    TOP("LD A,E");
    TOP("LD H,17H");
    TOP("LD B,H");
    TOP("LD C,H");
    TOP("LD D,H");
    TOP("LD E,H");
    TOP("LD H,H");
    TOP("LD L,H");
    TOP("LD A,H");
    TOP("LD L,18H");
    TOP("LD B,L");
    TOP("LD C,L");
    TOP("LD D,L");
    TOP("LD E,L");
    TOP("LD H,L");
    TOP("LD L,L");
    TOP("LD A,L");
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
    TOP("LD HL,1000H");
    TOP("LD A,33H");
    TOP("LD (HL),A");
    TOP("LD A,22H");
    TOP("LD B,(HL)");
    TOP("LD C,(HL)");
    TOP("LD D,(HL)");
    TOP("LD E,(HL)");
    TOP("LD H,(HL)");
    TOP("LD H,10H");
    TOP("LD L,(HL)");
    TOP("LD L,00H");
    TOP("LD A,(HL)");
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
    TOP("LD IX,1003H");
    TOP("LD A,12H");
    TOP("LD (IX+0),A");
    TOP("LD B,13H");
    TOP("LD (IX+1),B");
    TOP("LD C,14H");
    TOP("LD (IX+2),C");
    TOP("LD D,15H");
    TOP("LD (IX-1),D");
    TOP("LD E,16H");
    TOP("LD (IX-2),E");
    TOP("LD H,17H");
    TOP("LD (IX+3),H");
    TOP("LD L,18H");
    TOP("LD (IX-3),L");
    TOP("LD IY,1003H");
    TOP("LD A,12H");
    TOP("LD (IY+0),A");
    TOP("LD B,13H");
    TOP("LD (IY+1),B");
    TOP("LD C,14H");
    TOP("LD (IY+2),C");
    TOP("LD D,15H");
    TOP("LD (IY-1),D");
    TOP("LD E,16H");
    TOP("LD (IY-2),E");
    TOP("LD H,17H");
    TOP("LD (IY+3),H");
    TOP("LD L,18H");
    TOP("LD (IY-3),L");
}

/* LD (HL),r */
void LD_iHLi_r(void) {
    test("LD (HL),r");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,   // LD HL,0x1000
        0x3E, 0x12,         // LD A,0x12
        0x77,               // LD (HL),A
        0x06, 0x13,         // LD B,0x13
        0x70,               // LD (HL),B
        0x0E, 0x14,         // LD C,0x14
        0x71,               // LD (HL),C
        0x16, 0x15,         // LD D,0x15
        0x72,               // LD (HL),D
        0x1E, 0x16,         // LD E,0x16
        0x73,               // LD (HL),E
        0x74,               // LD (HL),H
        0x75,               // LD (HL),L
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1000H");
    TOP("LD A,12H");
    TOP("LD (HL),A");
    TOP("LD B,13H");
    TOP("LD (HL),B");
    TOP("LD C,14H");
    TOP("LD (HL),C");
    TOP("LD D,15H");
    TOP("LD (HL),D");
    TOP("LD E,16H");
    TOP("LD (HL),E");
    TOP("LD (HL),H");
    TOP("LD (HL),L");
}

/* LD (IX|IY+d),r */
void LD_iIXIYi_r(void) {
    test("LD (IX+d),r; LD (IY+d)");
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
    TOP("LD IX,1003H");
    TOP("LD A,12H");
    TOP("LD (IX+0),A");
    TOP("LD B,13H");
    TOP("LD (IX+1),B");
    TOP("LD C,14H");
    TOP("LD (IX+2),C");
    TOP("LD D,15H");
    TOP("LD (IX-1),D");
    TOP("LD E,16H");
    TOP("LD (IX-2),E");
    TOP("LD H,17H");
    TOP("LD (IX+3),H");
    TOP("LD L,18H");
    TOP("LD (IX-3),L");
    TOP("LD IY,1003H");
    TOP("LD A,12H");
    TOP("LD (IY+0),A");
    TOP("LD B,13H");
    TOP("LD (IY+1),B");
    TOP("LD C,14H");
    TOP("LD (IY+2),C");
    TOP("LD D,15H");
    TOP("LD (IY-1),D");
    TOP("LD E,16H");
    TOP("LD (IY-2),E");
    TOP("LD H,17H");
    TOP("LD (IY+3),H");
    TOP("LD L,18H");
    TOP("LD (IY-3),L");
}

/* LD (HL),n */
void LD_iHLi_n(void) {
    test("LD (HL),n");
    uint8_t prog[] = {
        0x21, 0x00, 0x20,   // LD HL,0x2000
        0x36, 0x33,         // LD (HL),0x33
        0x21, 0x00, 0x10,   // LD HL,0x1000
        0x36, 0x65,         // LD (HL),0x65
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,2000H");
    TOP("LD (HL),33H");
    TOP("LD HL,1000H");
    TOP("LD (HL),65H");
}

/* LD (IX|IY+d),n */
void LD_iIXIYi_n(void) {
    test("LD (IX+d),n; LD (IY+d),n");
    uint8_t prog[] = {
        0xDD, 0x21, 0x00, 0x20,     // LD IX,0x2000
        0xDD, 0x36, 0x02, 0x33,     // LD (IX+2),0x33
        0xDD, 0x36, 0xFE, 0x11,     // LD (IX-2),0x11
        0xFD, 0x21, 0x00, 0x10,     // LD IY,0x1000
        0xFD, 0x36, 0x01, 0x22,     // LD (IY+1),0x22
        0xFD, 0x36, 0xFF, 0x44,     // LD (IY-1),0x44
    };
    init(0, prog, sizeof(prog));
    TOP("LD IX,2000H");
    TOP("LD (IX+2),33H");
    TOP("LD (IX-2),11H");
    TOP("LD IY,1000H");
    TOP("LD (IY+1),22H");
    TOP("LD (IY-1),44H");
}

/* LD A,(BC); LD A,(DE); LD A,(nn) */
void LD_A_iBCDEnni(void) {
    test("LD A,(BC); LD A,(DE); LD A,(nn)");
    uint8_t prog[] = {
        0x01, 0x00, 0x10,   // LD BC,0x1000
        0x11, 0x01, 0x10,   // LD DE,0x1001
        0x0A,               // LD A,(BC)
        0x1A,               // LD A,(DE)
        0x3A, 0x02, 0x10,   // LD A,(0x1002)
    };
    init(0, prog, sizeof(prog));
    TOP("LD BC,1000H");
    TOP("LD DE,1001H");
    TOP("LD A,(BC)");
    TOP("LD A,(DE)");
    TOP("LD A,(1002H)");
}

/* LD (BC),A; LD (DE),A; LD (nn),A */
void LD_iBCDEnni_A(void) {
    test("LD (BC),A; LD (DE),A; LD (nn),A");
    uint8_t prog[] = {
        0x01, 0x00, 0x10,   // LD BC,0x1000
        0x11, 0x01, 0x10,   // LD DE,0x1001
        0x3E, 0x77,         // LD A,0x77
        0x02,               // LD (BC),A
        0x12,               // LD (DE),A
        0x32, 0x02, 0x10,   // LD (0x1002),A
    };
    init(0, prog, sizeof(prog));
    TOP("LD BC,1000H");
    TOP("LD DE,1001H");
    TOP("LD A,77H");
    TOP("LD (BC),A");
    TOP("LD (DE),A");
    TOP("LD (1002H),A");
}
/* LD dd,nn; LD IX,nn; LD IY,nn */
void LD_ddIXIY_nn(void) {
    test("LD dd,nn; LD IX,nn; LD IY,nn");
    uint8_t prog[] = {
        0x01, 0x34, 0x12,       // LD BC,0x1234
        0x11, 0x78, 0x56,       // LD DE,0x5678
        0x21, 0xBC, 0x9A,       // LD HL,0x9ABC
        0x31, 0x68, 0x13,       // LD SP,0x1368
        0xDD, 0x21, 0x21, 0x43, // LD IX,0x4321
        0xFD, 0x21, 0x65, 0x87, // LD IY,0x8765
    };
    init(0, prog, sizeof(prog));
    TOP("LD BC,1234H");
    TOP("LD DE,5678H");
    TOP("LD HL,9ABCH");
    TOP("LD SP,1368H");
    TOP("LD IX,4321H");
    TOP("LD IY,8765H");
}

/* LD HL,(nn); LD dd,(nn); LD IX,(nn); LD IY,(nn) */
void LD_HLddIXIY_inni(void) {
    test("LD dd,(nn); LD IX,(nn); LD IY,(nn)");
    uint8_t prog[] = {
        0x2A, 0x00, 0x10,           // LD HL,(0x1000)
        0xED, 0x4B, 0x01, 0x10,     // LD BC,(0x1001)
        0xED, 0x5B, 0x02, 0x10,     // LD DE,(0x1002)
        0xED, 0x6B, 0x03, 0x10,     // LD HL,(0x1003) undocumented 'long' version
        0xED, 0x7B, 0x04, 0x10,     // LD SP,(0x1004)
        0xDD, 0x2A, 0x05, 0x10,     // LD IX,(0x1005)
        0xFD, 0x2A, 0x06, 0x10,     // LD IY,(0x1006)
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,(1000H)");
    TOP("LD BC,(1001H)");
    TOP("LD DE,(1002H)");
    TOP("LD HL,(1003H)");
    TOP("LD SP,(1004H)");
    TOP("LD IX,(1005H)");
    TOP("LD IY,(1006H)");
}

/* LD (nn),HL; LD (nn),dd; LD (nn),IX; LD (nn),IY */
void LD_inni_HLddIXIY(void) {
    test("LD (nn),dd; LD (nn),IX; LD (nn),IY");
    uint8_t prog[] = {
        0x21, 0x01, 0x02,           // LD HL,0x0201
        0x22, 0x00, 0x10,           // LD (0x1000),HL
        0x01, 0x34, 0x12,           // LD BC,0x1234
        0xED, 0x43, 0x02, 0x10,     // LD (0x1002),BC
        0x11, 0x78, 0x56,           // LD DE,0x5678
        0xED, 0x53, 0x04, 0x10,     // LD (0x1004),DE
        0x21, 0xBC, 0x9A,           // LD HL,0x9ABC
        0xED, 0x63, 0x06, 0x10,     // LD (0x1006),HL undocumented 'long' version
        0x31, 0x68, 0x13,           // LD SP,0x1368
        0xED, 0x73, 0x08, 0x10,     // LD (0x1008),SP
        0xDD, 0x21, 0x21, 0x43,     // LD IX,0x4321
        0xDD, 0x22, 0x0A, 0x10,     // LD (0x100A),IX
        0xFD, 0x21, 0x65, 0x87,     // LD IY,0x8765
        0xFD, 0x22, 0x0C, 0x10,     // LD (0x100C),IY
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,0201H");
    TOP("LD (1000H),HL");
    TOP("LD BC,1234H");
    TOP("LD (1002H),BC");
    TOP("LD DE,5678H");
    TOP("LD (1004H),DE");
    TOP("LD HL,9ABCH");
    TOP("LD (1006H),HL");
    TOP("LD SP,1368H");
    TOP("LD (1008H),SP");
    TOP("LD IX,4321H");
    TOP("LD (100AH),IX");
    TOP("LD IY,8765H");
    TOP("LD (100CH),IY");
}

/* LD SP,HL; LD SP,IX; LD SP,IY */
void LD_SP_HLIXIY(void) {
    test("LD SP,HL; LD SP,IX; LD SP,IY");
    uint8_t prog[] = {
        0x21, 0x34, 0x12,           // LD HL,0x1234
        0xDD, 0x21, 0x78, 0x56,     // LD IX,0x5678
        0xFD, 0x21, 0xBC, 0x9A,     // LD IY,0x9ABC
        0xF9,                       // LD SP,HL
        0xDD, 0xF9,                 // LD SP,IX
        0xFD, 0xF9,                 // LD SP,IY
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1234H");
    TOP("LD IX,5678H");
    TOP("LD IY,9ABCH");
    TOP("LD SP,HL");
    TOP("LD SP,IX");
    TOP("LD SP,IY");
}

/* PUSH qq; PUSH IX; PUSH IY; POP qq; POP IX; POP IY */
void PUSH_POP_qqIXIY(void) {
    test("PUSH qq; PUSH IX; PUSH IY; POP qq; POP IX; POP IY");
    uint8_t prog[] = {
        0x01, 0x34, 0x12,       // LD BC,0x1234
        0x11, 0x78, 0x56,       // LD DE,0x5678
        0x21, 0xBC, 0x9A,       // LD HL,0x9ABC
        0x3E, 0xEF,             // LD A,0xEF
        0xDD, 0x21, 0x45, 0x23, // LD IX,0x2345
        0xFD, 0x21, 0x89, 0x67, // LD IY,0x6789
        0x31, 0x00, 0x01,       // LD SP,0x0100
        0xF5,                   // PUSH AF
        0xC5,                   // PUSH BC
        0xD5,                   // PUSH DE
        0xE5,                   // PUSH HL
        0xDD, 0xE5,             // PUSH IX
        0xFD, 0xE5,             // PUSH IY
        0xF1,                   // POP AF
        0xC1,                   // POP BC
        0xD1,                   // POP DE
        0xE1,                   // POP HL
        0xDD, 0xE1,             // POP IX
        0xFD, 0xE1,             // POP IY
    };
    init(0, prog, sizeof(prog));
    TOP("LD BC,1234H");
    TOP("LD DE,5678H");
    TOP("LD HL,9ABCH");
    TOP("LD A,EFH");
    TOP("LD IX,2345H");
    TOP("LD IY,6789H");
    TOP("LD SP,0100H");
    TOP("PUSH AF");
    TOP("PUSH BC");
    TOP("PUSH DE");
    TOP("PUSH HL");
    TOP("PUSH IX");
    TOP("PUSH IY");
    TOP("POP AF");
    TOP("POP BC");
    TOP("POP DE");
    TOP("POP HL");
    TOP("POP IX");
    TOP("POP IY");
}

/* EX DE,HL; EX AF,AF'; EX (SP),HL; EX (SP),IX; EX (SP),IY */
void EX(void) {
    test("EX DE,HL; EX AF,AF'; EX (SP),HL; EX (SP),IX; EX (SP),IY");
    uint8_t prog[] = {
        0x21, 0x34, 0x12,       // LD HL,0x1234
        0x11, 0x78, 0x56,       // LD DE,0x5678
        0xEB,                   // EX DE,HL
        0x3E, 0x11,             // LD A,0x11
        0x08,                   // EX AF,AF'
        0x3E, 0x22,             // LD A,0x22
        0x08,                   // EX AF,AF'
        0x01, 0xBC, 0x9A,       // LD BC,0x9ABC
        0xD9,                   // EXX
        0x21, 0x11, 0x11,       // LD HL,0x1111
        0x11, 0x22, 0x22,       // LD DE,0x2222
        0x01, 0x33, 0x33,       // LD BC,0x3333
        0xD9,                   // EXX
        0x31, 0x00, 0x01,       // LD SP,0x0100
        0xD5,                   // PUSH DE
        0xE3,                   // EX (SP),HL
        0xDD, 0x21, 0x99, 0x88, // LD IX,0x8899
        0xDD, 0xE3,             // EX (SP),IX
        0xFD, 0x21, 0x77, 0x66, // LD IY,0x6677
        0xFD, 0xE3,             // EX (SP),IY
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1234H");
    TOP("LD DE,5678H");
    TOP("EX DE,HL");
    TOP("LD A,11H");
    TOP("EX AF,AF'");
    TOP("LD A,22H");
    TOP("EX AF,AF'");
    TOP("LD BC,9ABCH");
    TOP("EXX");
    TOP("LD HL,1111H");
    TOP("LD DE,2222H");
    TOP("LD BC,3333H");
    TOP("EXX");
    TOP("LD SP,0100H");
    TOP("PUSH DE");
    TOP("EX (SP),HL");
    TOP("LD IX,8899H");
    TOP("EX (SP),IX");
    TOP("LD IY,6677H");
    TOP("EX (SP),IY");
}

/* ADD A,r; ADD A,n */
void ADD_A_rn(void) {
    test("ADD A,r; ADD A,n");
    uint8_t prog[] = {
        0x3E, 0x0F,     // LD A,0x0F
        0x87,           // ADD A,A
        0x06, 0xE0,     // LD B,0xE0
        0x80,           // ADD A,B
        0x3E, 0x81,     // LD A,0x81
        0x0E, 0x80,     // LD C,0x80
        0x81,           // ADD A,C
        0x16, 0xFF,     // LD D,0xFF
        0x82,           // ADD A,D
        0x1E, 0x40,     // LD E,0x40
        0x83,           // ADD A,E
        0x26, 0x80,     // LD H,0x80
        0x84,           // ADD A,H
        0x2E, 0x33,     // LD L,0x33
        0x85,           // ADD A,L
        0xC6, 0x44,     // ADD A,0x44
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,0FH");
    TOP("ADD A,A");
    TOP("LD B,E0H");
    TOP("ADD A,B");
    TOP("LD A,81H");
    TOP("LD C,80H");
    TOP("ADD A,C");
    TOP("LD D,FFH");
    TOP("ADD A,D");
    TOP("LD E,40H");
    TOP("ADD A,E");
    TOP("LD H,80H");
    TOP("ADD A,H");
    TOP("LD L,33H");
    TOP("ADD A,L");
    TOP("ADD A,44H");
}

/* ADD A,(HL); ADD A,(IX+d); ADD A,(IY+d) */
void ADD_A_iHLIXIYi(void) {
    test("ADD A,(HL); ADD A,(IX+d); ADD A,(IY+d)");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x00,             // LD A,0x00
        0x86,                   // ADD A,(HL)
        0xDD, 0x86, 0x01,       // ADD A,(IX+1)
        0xFD, 0x86, 0xFF,       // ADD A,(IY-1)
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1000H");
    TOP("LD IX,1000H");
    TOP("LD IY,1003H");
    TOP("LD A,00H");
    TOP("ADD A,(HL)");
    TOP("ADD A,(IX+1)");
    TOP("ADD A,(IY-1)");
}

/* ADC A,r; ADC A,n */
void ADC_A_rn(void) {
    test("ADC A,r; ADD A,n");
    uint8_t prog[] = {
        0x3E, 0x00,         // LD A,0x00
        0x06, 0x41,         // LD B,0x41
        0x0E, 0x61,         // LD C,0x61
        0x16, 0x81,         // LD D,0x81
        0x1E, 0x41,         // LD E,0x41
        0x26, 0x61,         // LD H,0x61
        0x2E, 0x81,         // LD L,0x81
        0x8F,               // ADC A,A
        0x88,               // ADC A,B
        0x89,               // ADC A,C
        0x8A,               // ADC A,D
        0x8B,               // ADC A,E
        0x8C,               // ADC A,H
        0x8D,               // ADC A,L
        0xCE, 0x01,         // ADC A,0x01
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,00H");
    TOP("LD B,41H");
    TOP("LD C,61H");
    TOP("LD D,81H");
    TOP("LD E,41H");
    TOP("LD H,61H");
    TOP("LD L,81H");
    TOP("ADC A,A");
    TOP("ADC A,B");
    TOP("ADC A,C");
    TOP("ADC A,D");
    TOP("ADC A,E");
    TOP("ADC A,H");
    TOP("ADC A,L");
    TOP("ADC A,01H");
}

/* ADC A,(HL); ADC A,(IX+d); ADC A,(IY+d) */
void ADC_A_iHLIXIYi(void) {
    test("ADC A,(HL); ADC A,(IX+d); ADC A,(IY+d)");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x00,             // LD A,0x00
        0x86,                   // ADD A,(HL)
        0xDD, 0x8E, 0x01,       // ADC A,(IX+1)
        0xFD, 0x8E, 0xFF,       // ADC A,(IY-1)
        0xDD, 0x8E, 0x03,       // ADC A,(IX+3)
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1000H");
    TOP("LD IX,1000H");
    TOP("LD IY,1003H");
    TOP("LD A,00H");
    TOP("ADD A,(HL)");
    TOP("ADC A,(IX+1)");
    TOP("ADC A,(IY-1)");
    TOP("ADC A,(IX+3)");
}

/* SUB A,r; SUB A,n */
void SUB_A_rn(void) {
    test("SUB A,r; SUB A,n");
    uint8_t prog[] = {
        0x3E, 0x04,     // LD A,0x04
        0x06, 0x01,     // LD B,0x01
        0x0E, 0xF8,     // LD C,0xF8
        0x16, 0x0F,     // LD D,0x0F
        0x1E, 0x79,     // LD E,0x79
        0x26, 0xC0,     // LD H,0xC0
        0x2E, 0xBF,     // LD L,0xBF
        0x97,           // SUB A,A
        0x90,           // SUB A,B
        0x91,           // SUB A,C
        0x92,           // SUB A,D
        0x93,           // SUB A,E
        0x94,           // SUB A,H
        0x95,           // SUB A,L
        0xD6, 0x01,     // SUB A,0x01
        0xD6, 0xFE,     // SUB A,0xFE
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,04H");
    TOP("LD B,01H");
    TOP("LD C,F8H");
    TOP("LD D,0FH");
    TOP("LD E,79H");
    TOP("LD H,C0H");
    TOP("LD L,BFH");
    TOP("SUB A");
    TOP("SUB B");
    TOP("SUB C");
    TOP("SUB D");
    TOP("SUB E");
    TOP("SUB H");
    TOP("SUB L");
    TOP("SUB 01H");
    TOP("SUB FEH");
}

/* SUB A,(HL); SUB A,(IX+d); SUB A,(IY+d) */
void SUB_A_iHLIXIYi(void) {
    test("SUB A,(HL); SUB A,(IX+d); SUB A,(IY+d)");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x00,             // LD A,0x00
        0x96,                   // SUB A,(HL)
        0xDD, 0x96, 0x01,       // SUB A,(IX+1)
        0xFD, 0x96, 0xFE,       // SUB A,(IY-2)
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1000H");
    TOP("LD IX,1000H");
    TOP("LD IY,1003H");
    TOP("LD A,00H");
    TOP("SUB (HL)");
    TOP("SUB (IX+1)");
    TOP("SUB (IY-2)");
}

/* SBC A,r; SBC A,n */
void SBC_A_rn(void) {
    test("SBC A,r; SBC A,n");
    uint8_t prog[] = {
        0x3E, 0x04,     // LD A,0x04
        0x06, 0x01,     // LD B,0x01
        0x0E, 0xF8,     // LD C,0xF8
        0x16, 0x0F,     // LD D,0x0F
        0x1E, 0x79,     // LD E,0x79
        0x26, 0xC0,     // LD H,0xC0
        0x2E, 0xBF,     // LD L,0xBF
        0x97,           // SUB A,A
        0x98,           // SBC A,B
        0x99,           // SBC A,C
        0x9A,           // SBC A,D
        0x9B,           // SBC A,E
        0x9C,           // SBC A,H
        0x9D,           // SBC A,L
        0xDE, 0x01,     // SBC A,0x01
        0xDE, 0xFE,     // SBC A,0xFE
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,04H");
    TOP("LD B,01H");
    TOP("LD C,F8H");
    TOP("LD D,0FH");
    TOP("LD E,79H");
    TOP("LD H,C0H");
    TOP("LD L,BFH");
    TOP("SUB A");
    TOP("SBC A,B");
    TOP("SBC A,C");
    TOP("SBC A,D");
    TOP("SBC A,E");
    TOP("SBC A,H");
    TOP("SBC A,L");
    TOP("SBC A,01H");
    TOP("SBC A,FEH");
}

/* SBC A,(HL); SBC A,(IX+d); SBC A,(IY+d) */
void SBC_A_iHLIXIYi(void) {
    test("SBC A,(HL); SBC A,(IX+d); SBC A,(IY+d)");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x00,             // LD A,0x00
        0x9E,                   // SBC A,(HL)
        0xDD, 0x9E, 0x01,       // SBC A,(IX+1)
        0xFD, 0x9E, 0xFE,       // SBC A,(IY-2)
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1000H");
    TOP("LD IX,1000H");
    TOP("LD IY,1003H");
    TOP("LD A,00H");
    TOP("SBC A,(HL)");
    TOP("SBC A,(IX+1)");
    TOP("SBC A,(IY-2)");
}

/* CP A,r; CP A,n */
void CP_A_rn(void) {
    test("CP A,r; CP A,n");
    uint8_t prog[] = {
        0x3E, 0x04,     // LD A,0x04
        0x06, 0x05,     // LD B,0x05
        0x0E, 0x03,     // LD C,0x03
        0x16, 0xff,     // LD D,0xff
        0x1E, 0xaa,     // LD E,0xaa
        0x26, 0x80,     // LD H,0x80
        0x2E, 0x7f,     // LD L,0x7f
        0xBF,           // CP A
        0xB8,           // CP B
        0xB9,           // CP C
        0xBA,           // CP D
        0xBB,           // CP E
        0xBC,           // CP H
        0xBD,           // CP L
        0xFE, 0x04,     // CP 0x04
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,04H");
    TOP("LD B,05H");
    TOP("LD C,03H");
    TOP("LD D,FFH");
    TOP("LD E,AAH");
    TOP("LD H,80H");
    TOP("LD L,7FH");
    TOP("CP A");
    TOP("CP B");
    TOP("CP C");
    TOP("CP D");
    TOP("CP E");
    TOP("CP H");
    TOP("CP L");
    TOP("CP 04H");
}
/* CP A,(HL); CP A,(IX+d); CP A,(IY+d) */
void CP_A_iHLIXIYi(void) {
    test("CP A,(HL); CP A,(IX+d); CP A,(IY+d)");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x41,             // LD A,0x41
        0xBE,                   // CP (HL)
        0xDD, 0xBE, 0x01,       // CP (IX+1)
        0xFD, 0xBE, 0xFF,       // CP (IY-1)
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1000H");
    TOP("LD IX,1000H");
    TOP("LD IY,1003H");
    TOP("LD A,41H");
    TOP("CP (HL)");
    TOP("CP (IX+1)");
    TOP("CP (IY-1)");
}

/* AND A,r; AND A,n */
void AND_A_rn(void) {
    test("AND A,r; AND A,n");
    uint8_t prog[] = {
        0x3E, 0xFF,             // LD A,0xFF
        0x06, 0x01,             // LD B,0x01
        0x0E, 0x03,             // LD C,0x03
        0x16, 0x04,             // LD D,0x04
        0x1E, 0x08,             // LD E,0x08
        0x26, 0x10,             // LD H,0x10
        0x2E, 0x20,             // LD L,0x20
        0xA0,                   // AND B
        0xF6, 0xFF,             // OR 0xFF
        0xA1,                   // AND C
        0xF6, 0xFF,             // OR 0xFF
        0xA2,                   // AND D
        0xF6, 0xFF,             // OR 0xFF
        0xA3,                   // AND E
        0xF6, 0xFF,             // OR 0xFF
        0xA4,                   // AND H
        0xF6, 0xFF,             // OR 0xFF
        0xA5,                   // AND L
        0xF6, 0xFF,             // OR 0xFF
        0xE6, 0x40,             // AND 0x40
        0xF6, 0xFF,             // OR 0xFF
        0xE6, 0xAA,             // AND 0xAA
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,FFH")
    TOP("LD B,01H")
    TOP("LD C,03H")
    TOP("LD D,04H")
    TOP("LD E,08H")
    TOP("LD H,10H")
    TOP("LD L,20H")
    TOP("AND B")
    TOP("OR FFH")
    TOP("AND C")
    TOP("OR FFH")
    TOP("AND D")
    TOP("OR FFH")
    TOP("AND E")
    TOP("OR FFH")
    TOP("AND H")
    TOP("OR FFH")
    TOP("AND L")
    TOP("OR FFH")
    TOP("AND 40H")
    TOP("OR FFH")
    TOP("AND AAH")
}

/* AND A,(HL); AND A,(IX+d); AND A,(IY+d) */
void AND_A_iHLIXIYi(void) {
    test("AND A,(HL); AND A,(IX+d); AND A,(IY+d)");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0x3E, 0xFF,                 // LD A,0xFF
        0xA6,                       // AND (HL)
        0xDD, 0xA6, 0x01,           // AND (IX+1)
        0xFD, 0xA6, 0xFF,           // AND (IY-1)
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1000H");
    TOP("LD IX,1000H");
    TOP("LD IY,1003H");
    TOP("LD A,FFH");
    TOP("AND (HL)");
    TOP("AND (IX+1)");
    TOP("AND (IY-1)");
}

/* XOR A,r; XOR A,n */
void XOR_A_rn(void) {
    test("XOR A,r; XOR A,n");
    uint8_t prog[] = {
        0x97,           // SUB A
        0x06, 0x01,     // LD B,0x01
        0x0E, 0x03,     // LD C,0x03
        0x16, 0x07,     // LD D,0x07
        0x1E, 0x0F,     // LD E,0x0F
        0x26, 0x1F,     // LD H,0x1F
        0x2E, 0x3F,     // LD L,0x3F
        0xAF,           // XOR A
        0xA8,           // XOR B
        0xA9,           // XOR C
        0xAA,           // XOR D
        0xAB,           // XOR E
        0xAC,           // XOR H
        0xAD,           // XOR L
        0xEE, 0x7F,     // XOR 0x7F
        0xEE, 0xFF,     // XOR 0xFF
    };
    init(0, prog, sizeof(prog));
    TOP("SUB A");
    TOP("LD B,01H");
    TOP("LD C,03H");
    TOP("LD D,07H");
    TOP("LD E,0FH");
    TOP("LD H,1FH");
    TOP("LD L,3FH");
    TOP("XOR A");
    TOP("XOR B");
    TOP("XOR C");
    TOP("XOR D");
    TOP("XOR E");
    TOP("XOR H");
    TOP("XOR L");
    TOP("XOR 7FH");
    TOP("XOR FFH");
}

/* OR A,r; OR A,n */
void OR_A_rn(void) {
    test("OR A,r; OR A,n");
    uint8_t prog[] = {
        0x97,           // SUB A
        0x06, 0x01,     // LD B,0x01
        0x0E, 0x02,     // LD C,0x02
        0x16, 0x04,     // LD D,0x04
        0x1E, 0x08,     // LD E,0x08
        0x26, 0x10,     // LD H,0x10
        0x2E, 0x20,     // LD L,0x20
        0xB7,           // OR A
        0xB0,           // OR B
        0xB1,           // OR C
        0xB2,           // OR D
        0xB3,           // OR E
        0xB4,           // OR H
        0xB5,           // OR L
        0xF6, 0x40,     // OR 0x40
        0xF6, 0x80,     // OR 0x80
    };
    init(0, prog, sizeof(prog));
    TOP("SUB A");
    TOP("LD B,01H");
    TOP("LD C,02H");
    TOP("LD D,04H");
    TOP("LD E,08H");
    TOP("LD H,10H");
    TOP("LD L,20H");
    TOP("OR A");
    TOP("OR B");
    TOP("OR C");
    TOP("OR D");
    TOP("OR E");
    TOP("OR H");
    TOP("OR L");
    TOP("OR 40H");
    TOP("OR 80H");
}

/* OR A,(HL); OR A,(IX+d); OR A,(IY+d) */
/* XOR A,(HL); XOR A,(IX+d); XOR A,(IY+d) */
void OR_XOR_A_iHLIXIYi(void) {
    test("OR/XOR A,(HL); OR/XOR A,(IX+d); OR/XOR A,(IY+d)");
    uint8_t prog[] = {
        0x3E, 0x00,                 // LD A,0x00
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0xB6,                       // OR (HL)
        0xDD, 0xB6, 0x01,           // OR (IX+1)
        0xFD, 0xB6, 0xFF,           // OR (IY-1)
        0xAE,                       // XOR (HL)
        0xDD, 0xAE, 0x01,           // XOR (IX+1)
        0xFD, 0xAE, 0xFF,           // XOR (IY-1)
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,00H");
    TOP("LD HL,1000H");
    TOP("LD IX,1000H");
    TOP("LD IY,1003H");
    TOP("OR (HL)");
    TOP("OR (IX+1)");
    TOP("OR (IY-1)");
    TOP("XOR (HL)");
    TOP("XOR (IX+1)");
    TOP("XOR (IY-1)");
}

/* INC r; DEC r */
void INC_DEC_r(void) {
    test("INC r; DEC r");
    uint8_t prog[] = {
        0x3e, 0x00,         // LD A,0x00
        0x06, 0xFF,         // LD B,0xFF
        0x0e, 0x0F,         // LD C,0x0F
        0x16, 0x0E,         // LD D,0x0E
        0x1E, 0x7F,         // LD E,0x7F
        0x26, 0x3E,         // LD H,0x3E
        0x2E, 0x23,         // LD L,0x23
        0x3C,               // INC A
        0x3D,               // DEC A
        0x04,               // INC B
        0x05,               // DEC B
        0x0C,               // INC C
        0x0D,               // DEC C
        0x14,               // INC D
        0x15,               // DEC D
        0xFE, 0x01,         // CP 0x01  // set carry flag (should be preserved)
        0x1C,               // INC E
        0x1D,               // DEC E
        0x24,               // INC H
        0x25,               // DEC H
        0x2C,               // INC L
        0x2D,               // DEC L
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,00H");
    TOP("LD B,FFH");
    TOP("LD C,0FH");
    TOP("LD D,0EH");
    TOP("LD E,7FH");
    TOP("LD H,3EH");
    TOP("LD L,23H");
    TOP("INC A");
    TOP("DEC A");
    TOP("INC B");
    TOP("DEC B");
    TOP("INC C");
    TOP("DEC C");
    TOP("INC D");
    TOP("DEC D");
    TOP("CP 01H");
    TOP("INC E");
    TOP("DEC E");
    TOP("INC H");
    TOP("DEC H");
    TOP("INC L");
    TOP("DEC L");
}

/* INC (HL); INC (IX+d); INC (IY+d); DEC (HL); DEC (IX+d); DEC (IY+d) */
void INC_DEC_iHLIXIYi(void) {
    test("INC/DEC (HL)/(IX+d)/(IY+d)");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0x35,                       // DEC (HL)
        0x34,                       // INC (HL)
        0xDD, 0x34, 0x01,           // INC (IX+1)
        0xDD, 0x35, 0x01,           // DEC (IX+1)
        0xFD, 0x34, 0xFF,           // INC (IY-1)
        0xFD, 0x35, 0xFF,           // DEC (IY-1)
    };
    init(0, prog, sizeof(prog));
    TOP("LD HL,1000H");
    TOP("LD IX,1000H");
    TOP("LD IY,1003H");
    TOP("DEC (HL)");
    TOP("INC (HL)");
    TOP("INC (IX+1)");
    TOP("DEC (IX+1)");
    TOP("INC (IY-1)");
    TOP("DEC (IY-1)");
}

/* INC ss; INC IX; INC IY; DEC ss; DEC IX; DEC IY */
void INC_DEC_ssIXIY(void) {
    test("INC/DEC ss/IX/IY");
    uint8_t prog[] = {
        0x01, 0x00, 0x00,       // LD BC,0x0000
        0x11, 0xFF, 0xFF,       // LD DE,0xffff
        0x21, 0xFF, 0x00,       // LD HL,0x00ff
        0x31, 0x11, 0x11,       // LD SP,0x1111
        0xDD, 0x21, 0xFF, 0x0F, // LD IX,0x0fff
        0xFD, 0x21, 0x34, 0x12, // LD IY,0x1234
        0x0B,                   // DEC BC
        0x03,                   // INC BC
        0x13,                   // INC DE
        0x1B,                   // DEC DE
        0x23,                   // INC HL
        0x2B,                   // DEC HL
        0x33,                   // INC SP
        0x3B,                   // DEC SP
        0xDD, 0x23,             // INC IX
        0xDD, 0x2B,             // DEC IX
        0xFD, 0x23,             // INC IY
        0xFD, 0x2B,             // DEC IY
    };
    init(0, prog, sizeof(prog));
    TOP("LD BC,0000H");
    TOP("LD DE,FFFFH");
    TOP("LD HL,00FFH");
    TOP("LD SP,1111H");
    TOP("LD IX,0FFFH");
    TOP("LD IY,1234H");
    TOP("DEC BC");
    TOP("INC BC");
    TOP("INC DE");
    TOP("DEC DE");
    TOP("INC HL");
    TOP("DEC HL");
    TOP("INC SP");
    TOP("DEC SP");
    TOP("INC IX");
    TOP("DEC IX");
    TOP("INC IY");
    TOP("DEC IY");
}

/* RLCA; RLA; RRCA; RRA */
void RLCA_RLA_RRCA_RRA(void) {
    test("RLCA; RLA; RRCA; RRA");
    uint8_t prog[] = {
        0x3E, 0xA0,     // LD A,0xA0
        0x07,           // RLCA
        0x07,           // RLCA
        0x0F,           // RRCA
        0x0F,           // RRCA
        0x17,           // RLA
        0x17,           // RLA
        0x1F,           // RRA
        0x1F,           // RRA
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,A0H");
    TOP("RLCA");
    TOP("RLCA");
    TOP("RRCA");
    TOP("RRCA");
    TOP("RLA");
    TOP("RLA");
    TOP("RRA");
    TOP("RRA");
}

/* RLC r; RL r; RRC r; RR r */
void RLC_RL_RRC_RR_r(void) {
    test("RLC r; RL r; RRC r; RR r");
    uint8_t prog[] = {
        0x3E, 0x01,     // LD A,0x01
        0x06, 0xFF,     // LD B,0xFF
        0x0E, 0x03,     // LD C,0x03
        0x16, 0xFE,     // LD D,0xFE
        0x1E, 0x11,     // LD E,0x11
        0x26, 0x3F,     // LD H,0x3F
        0x2E, 0x70,     // LD L,0x70
        0xCB, 0x0F,     // RRC A
        0xCB, 0x07,     // RLC A
        0xCB, 0x08,     // RRC B
        0xCB, 0x00,     // RLC B
        0xCB, 0x01,     // RLC C
        0xCB, 0x09,     // RRC C
        0xCB, 0x02,     // RLC D
        0xCB, 0x0A,     // RRC D
        0xCB, 0x0B,     // RRC E
        0xCB, 0x03,     // RLC E
        0xCB, 0x04,     // RLC H
        0xCB, 0x0C,     // RCC H
        0xCB, 0x05,     // RLC L
        0xCB, 0x0D,     // RRC L
        0xCB, 0x1F,     // RR A
        0xCB, 0x17,     // RL A
        0xCB, 0x18,     // RR B
        0xCB, 0x10,     // RL B
        0xCB, 0x11,     // RL C
        0xCB, 0x19,     // RR C
        0xCB, 0x12,     // RL D
        0xCB, 0x1A,     // RR D
        0xCB, 0x1B,     // RR E
        0xCB, 0x13,     // RL E
        0xCB, 0x14,     // RL H
        0xCB, 0x1C,     // RR H
        0xCB, 0x15,     // RL L
        0xCB, 0x1D,     // RR L
    };
    init(0, prog, sizeof(prog));
    TOP("LD A,01H");
    TOP("LD B,FFH");
    TOP("LD C,03H");
    TOP("LD D,FEH");
    TOP("LD E,11H");
    TOP("LD H,3FH");
    TOP("LD L,70H");
    TOP("RRC A");
    TOP("RLC A");
    TOP("RRC B");
    TOP("RLC B");
    TOP("RLC C");
    TOP("RRC C");
    TOP("RLC D");
    TOP("RRC D");
    TOP("RRC E");
    TOP("RLC E");
    TOP("RLC H");
    TOP("RRC H");
    TOP("RLC L");
    TOP("RRC L");
    TOP("RR A");
    TOP("RL A");
    TOP("RR B");
    TOP("RL B");
    TOP("RL C");
    TOP("RR C");
    TOP("RL D");
    TOP("RR D");
    TOP("RR E");
    TOP("RL E");
    TOP("RL H");
    TOP("RR H");
    TOP("RL L");
    TOP("RR L");
}

int main() {
    test_begin("z80dasm-test");
    LD_A_RI();
    LD_IR_A();
    LD_r_sn();
    LD_r_iHLi();
    LD_r_iIXIYi();
    LD_iHLi_r();
    LD_iIXIYi_r();
    LD_iHLi_n();
    LD_iIXIYi_n();
    LD_A_iBCDEnni();
    LD_iBCDEnni_A();
    LD_ddIXIY_nn();
    LD_HLddIXIY_inni();
    LD_inni_HLddIXIY();
    LD_SP_HLIXIY();
    PUSH_POP_qqIXIY();
    EX();
    ADD_A_rn();
    ADD_A_iHLIXIYi();
    ADC_A_rn();
    ADC_A_iHLIXIYi();
    SUB_A_rn();
    SUB_A_iHLIXIYi();
    SBC_A_rn();
    SBC_A_iHLIXIYi();
    CP_A_rn();
    CP_A_iHLIXIYi();
    AND_A_rn();
    AND_A_iHLIXIYi();
    XOR_A_rn();
    OR_A_rn();
    OR_XOR_A_iHLIXIYi();
    INC_DEC_r();
    INC_DEC_iHLIXIYi();
    INC_DEC_ssIXIY();
    RLCA_RLA_RRCA_RRA();
    RLC_RL_RRC_RR_r();
    return test_end();
}

