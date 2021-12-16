//------------------------------------------------------------------------------
//  m6502dasm-test.c
//------------------------------------------------------------------------------
#define CHIPS_UTIL_IMPL
#include "util/m6502dasm.h"
#include "utest.h"
#include <string.h>
#include <ctype.h>

#define T(b) ASSERT_TRUE(b)
#define TOP(str) T(op(str))

typedef struct {
    const uint8_t* ptr;
    const uint8_t* end_ptr;
    uint16_t pc;
    size_t str_pos;
    char str[32];
} ctx_t;
static ctx_t ctx;

static void init(uint16_t pc, const uint8_t* ptr, size_t len) {
    ctx.ptr = ptr;
    ctx.end_ptr = ptr + len;
    ctx.pc = pc;
    ctx.str_pos = 0;
    memset(ctx.str, 0, sizeof(ctx.str));
}

static uint8_t in_cb(void* user_data) {
    (void)user_data;
    if (ctx.ptr < ctx.end_ptr) {
        return *ctx.ptr++;
    }
    else {
        return 0;
    }
}

static void out_cb(char c, void* user_data) {
    (void)user_data;
    if ((ctx.str_pos + 1) < sizeof(ctx.str)) {
        ctx.str[ctx.str_pos++] = c;
        ctx.str[ctx.str_pos] = 0;
    }
}

static bool op(const char* res) {
    ctx.str_pos = 0;
    ctx.pc = m6502dasm_op(ctx.pc, in_cb, out_cb, 0);
    for (int i = 0; i < 32; i++) {
        ctx.str[i] = toupper(ctx.str[i]);
    }
    return 0 == strcmp(ctx.str, res);
}

UTEST(m6502dasm, LDA) {
    uint8_t prog[] = {
        // immediate
        0xA9, 0x00,         // LDA #$00
        0xA9, 0x01,         // LDA #$01
        0xA9, 0x00,         // LDA #$00
        0xA9, 0x80,         // LDA #$80

        // zero page
        0xA5, 0x02,         // LDA $02
        0xA5, 0x03,         // LDA $03
        0xA5, 0x80,         // LDA $80
        0xA5, 0xFF,         // LDA $FF

        // absolute
        0xAD, 0x00, 0x10,   // LDA $1000
        0xAD, 0xFF, 0xFF,   // LDA $FFFF
        0xAD, 0x21, 0x00,   // LDA $0021

        // zero page,X
        0xA2, 0x0F,         // LDX #$0F
        0xB5, 0x10,         // LDA $10,X    => 0x1F
        0xB5, 0xF8,         // LDA $F8,X    => 0x07
        0xB5, 0x78,         // LDA $78,X    => 0x87

        // absolute,X
        0xBD, 0xF1, 0x0F,   // LDA $0x0FF1,X    => 0x1000
        0xBD, 0xF0, 0xFF,   // LDA $0xFFF0,X    => 0xFFFF
        0xBD, 0x12, 0x00,   // LDA $0x0012,X    => 0x0021

        // absolute,Y
        0xA0, 0xF0,         // LDY #$F0
        0xB9, 0x10, 0x0F,   // LDA $0x0F10,Y    => 0x1000
        0xB9, 0x0F, 0xFF,   // LDA $0xFF0F,Y    => 0xFFFF
        0xB9, 0x31, 0xFF,   // LDA $0xFF31,Y    => 0x0021

        // indirect,X
        0xA1, 0xF0,         // LDA ($F0,X)  => 0xFF, second byte in 0x00 => 0x1234
        0xA1, 0x70,         // LDA ($70,X)  => 0x70 => 0x4321

        // indirect,Y
        0xB1, 0xFF,         // LDA ($FF),Y  => 0x1234+0xF0 => 0x1324
        0xB1, 0x7F,         // LDA ($7F),Y  => 0x4321+0xF0 => 0x4411
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$00");
    TOP("LDA #$01");
    TOP("LDA #$00");
    TOP("LDA #$80");
    TOP("LDA $02");
    TOP("LDA $03");
    TOP("LDA $80");
    TOP("LDA $FF");
    TOP("LDA $1000");
    TOP("LDA $FFFF");
    TOP("LDA $0021");
    TOP("LDX #$0F");
    TOP("LDA $10,X");
    TOP("LDA $F8,X");
    TOP("LDA $78,X");
    TOP("LDA $0FF1,X");
    TOP("LDA $FFF0,X");
    TOP("LDA $0012,X");
    TOP("LDY #$F0");
    TOP("LDA $0F10,Y");
    TOP("LDA $FF0F,Y");
    TOP("LDA $FF31,Y");
    TOP("LDA ($F0,X)");
    TOP("LDA ($70,X)");
    TOP("LDA ($FF),Y");
    TOP("LDA ($7F),Y");
}

UTEST(m6502dasm, LDX) {
    uint8_t prog[] = {
        // immediate
        0xA2, 0x00,         // LDX #$00
        0xA2, 0x01,         // LDX #$01
        0xA2, 0x00,         // LDX #$00
        0xA2, 0x80,         // LDX #$80

        // zero page
        0xA6, 0x02,         // LDX $02
        0xA6, 0x03,         // LDX $03
        0xA6, 0x80,         // LDX $80
        0xA6, 0xFF,         // LDX $FF

        // absolute
        0xAE, 0x00, 0x10,   // LDX $1000
        0xAE, 0xFF, 0xFF,   // LDX $FFFF
        0xAE, 0x21, 0x00,   // LDX $0021

        // zero page,Y
        0xA0, 0x0F,         // LDY #$0F
        0xB6, 0x10,         // LDX $10,Y    => 0x1F
        0xB6, 0xF8,         // LDX $F8,Y    => 0x07
        0xB6, 0x78,         // LDX $78,Y    => 0x87

        // absolute,Y
        0xA0, 0xF0,         // LDY #$F0
        0xBE, 0x10, 0x0F,   // LDX $0F10,Y    => 0x1000
        0xBE, 0x0F, 0xFF,   // LDX $FF0F,Y    => 0xFFFF
        0xBE, 0x31, 0xFF,   // LDX $FF31,Y    => 0x0021
    };
    init(0, prog, sizeof(prog));
    TOP("LDX #$00")
    TOP("LDX #$01")
    TOP("LDX #$00")
    TOP("LDX #$80")
    TOP("LDX $02")
    TOP("LDX $03")
    TOP("LDX $80")
    TOP("LDX $FF")
    TOP("LDX $1000")
    TOP("LDX $FFFF")
    TOP("LDX $0021")
    TOP("LDY #$0F")
    TOP("LDX $10,Y")
    TOP("LDX $F8,Y")
    TOP("LDX $78,Y")
    TOP("LDY #$F0")
    TOP("LDX $0F10,Y")
    TOP("LDX $FF0F,Y")
    TOP("LDX $FF31,Y")
}

UTEST(m6502dasm, LDY) {
    uint8_t prog[] = {
        // immediate
        0xA0, 0x00,         // LDY #$00
        0xA0, 0x01,         // LDY #$01
        0xA0, 0x00,         // LDY #$00
        0xA0, 0x80,         // LDY #$80

        // zero page
        0xA4, 0x02,         // LDY $02
        0xA4, 0x03,         // LDY $03
        0xA4, 0x80,         // LDY $80
        0xA4, 0xFF,         // LDY $FF

        // absolute
        0xAC, 0x00, 0x10,   // LDY $1000
        0xAC, 0xFF, 0xFF,   // LDY $FFFF
        0xAC, 0x21, 0x00,   // LDY $0021

        // zero page,X
        0xA2, 0x0F,         // LDX #$0F
        0xB4, 0x10,         // LDY $10,X    => 0x1F
        0xB4, 0xF8,         // LDY $F8,X    => 0x07
        0xB4, 0x78,         // LDY $78,X    => 0x87

        // absolute,X
        0xA2, 0xF0,         // LDX #$F0
        0xBC, 0x10, 0x0F,   // LDY $0F10,X    => 0x1000
        0xBC, 0x0F, 0xFF,   // LDY $FF0F,X    => 0xFFFF
        0xBC, 0x31, 0xFF,   // LDY $FF31,X    => 0x0021
    };
    init(0, prog, sizeof(prog));
    TOP("LDY #$00");
    TOP("LDY #$01");
    TOP("LDY #$00");
    TOP("LDY #$80");
    TOP("LDY $02");
    TOP("LDY $03");
    TOP("LDY $80");
    TOP("LDY $FF");
    TOP("LDY $1000");
    TOP("LDY $FFFF");
    TOP("LDY $0021");
    TOP("LDX #$0F");
    TOP("LDY $10,X");
    TOP("LDY $F8,X");
    TOP("LDY $78,X");
    TOP("LDX #$F0");
    TOP("LDY $0F10,X");
    TOP("LDY $FF0F,X");
    TOP("LDY $FF31,X");
}

UTEST(m6502dasm, STA) {
    uint8_t prog[] = {
        0xA9, 0x23,             // LDA #$23
        0xA2, 0x10,             // LDX #$10
        0xA0, 0xC0,             // LDY #$C0
        0x85, 0x10,             // STA $10
        0x8D, 0x34, 0x12,       // STA $1234
        0x95, 0x10,             // STA $10,X
        0x9D, 0x00, 0x20,       // STA $2000,X
        0x99, 0x00, 0x20,       // STA $2000,Y
        0x81, 0x10,             // STA ($10,X)
        0x91, 0x20,             // STA ($20),Y
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$23");
    TOP("LDX #$10");
    TOP("LDY #$C0");
    TOP("STA $10");
    TOP("STA $1234");
    TOP("STA $10,X");
    TOP("STA $2000,X");
    TOP("STA $2000,Y");
    TOP("STA ($10,X)");
    TOP("STA ($20),Y");
}

UTEST(m6502dasm, STX) {
    uint8_t prog[] = {
        0xA2, 0x23,             // LDX #$23
        0xA0, 0x10,             // LDY #$10
        0x86, 0x10,             // STX $10
        0x8E, 0x34, 0x12,       // STX $1234
        0x96, 0x10,             // STX $10,Y
    };
    init(0, prog, sizeof(prog));
    TOP("LDX #$23");
    TOP("LDY #$10");
    TOP("STX $10");
    TOP("STX $1234");
    TOP("STX $10,Y");
}

UTEST(m6502dasm, STY) {
    uint8_t prog[] = {
        0xA0, 0x23,             // LDY #$23
        0xA2, 0x10,             // LDX #$10
        0x84, 0x10,             // STY $10
        0x8C, 0x34, 0x12,       // STY $1234
        0x94, 0x10,             // STY $10,X
    };
    init(0, prog, sizeof(prog));
    TOP("LDY #$23");
    TOP("LDX #$10");
    TOP("STY $10");
    TOP("STY $1234");
    TOP("STY $10,X");
}

UTEST(m6502dasm, TAX_TXA) {
    uint8_t prog[] = {
        0xA9, 0x00,     // LDA #$00
        0xA2, 0x10,     // LDX #$10
        0xAA,           // TAX
        0xA9, 0xF0,     // LDA #$F0
        0x8A,           // TXA
        0xA9, 0xF0,     // LDA #$F0
        0xA2, 0x00,     // LDX #$00
        0xAA,           // TAX
        0xA9, 0x01,     // LDA #$01
        0x8A,           // TXA
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$00");
    TOP("LDX #$10");
    TOP("TAX");
    TOP("LDA #$F0");
    TOP("TXA");
    TOP("LDA #$F0");
    TOP("LDX #$00");
    TOP("TAX");
    TOP("LDA #$01");
    TOP("TXA");
}

UTEST(m6502dasm, TAY_TYA) {
    uint8_t prog[] = {
        0xA9, 0x00,     // LDA #$00
        0xA0, 0x10,     // LDY #$10
        0xA8,           // TAY
        0xA9, 0xF0,     // LDA #$F0
        0x98,           // TYA
        0xA9, 0xF0,     // LDA #$F0
        0xA0, 0x00,     // LDY #$00
        0xA8,           // TAY
        0xA9, 0x01,     // LDA #$01
        0x98,           // TYA
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$00");
    TOP("LDY #$10");
    TOP("TAY");
    TOP("LDA #$F0");
    TOP("TYA");
    TOP("LDA #$F0");
    TOP("LDY #$00");
    TOP("TAY");
    TOP("LDA #$01");
    TOP("TYA");
}

UTEST(m6502dasm, DEX_INX_DEY_INY) {
    uint8_t prog[] = {
        0xA2, 0x01,     // LDX #$01
        0xCA,           // DEX
        0xCA,           // DEX
        0xE8,           // INX
        0xE8,           // INX
        0xA0, 0x01,     // LDY #$01
        0x88,           // DEY
        0x88,           // DEY
        0xC8,           // INY
        0xC8,           // INY
    };
    init(0, prog, sizeof(prog));
    TOP("LDX #$01");
    TOP("DEX");
    TOP("DEX");
    TOP("INX");
    TOP("INX");
    TOP("LDY #$01");
    TOP("DEY");
    TOP("DEY");
    TOP("INY");
    TOP("INY");
}

UTEST(m6502dasm, TXS_TSX) {
    uint8_t prog[] = {
        0xA2, 0xAA,     // LDX #$AA
        0xA9, 0x00,     // LDA #$00
        0x9A,           // TXS
        0xAA,           // TAX
        0xBA,           // TSX
    };
    init(0, prog, sizeof(prog));
    TOP("LDX #$AA");
    TOP("LDA #$00");
    TOP("TXS");
    TOP("TAX");
    TOP("TSX");
}

UTEST(m6502dasm, ORA) {
    uint8_t prog[] = {
        0xA9, 0x00,         // LDA #$00
        0xA2, 0x01,         // LDX #$01
        0xA0, 0x02,         // LDY #$02
        0x09, 0x00,         // ORA #$00
        0x05, 0x10,         // ORA $10
        0x15, 0x10,         // ORA $10,X
        0x0d, 0x00, 0x10,   // ORA $1000
        0x1d, 0x00, 0x10,   // ORA $1000,X
        0x19, 0x00, 0x10,   // ORA $1000,Y
        0x01, 0x22,         // ORA ($22,X)
        0x11, 0x20,         // ORA ($20),Y
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$00");
    TOP("LDX #$01");
    TOP("LDY #$02");
    TOP("ORA #$00");
    TOP("ORA $10");
    TOP("ORA $10,X");
    TOP("ORA $1000");
    TOP("ORA $1000,X");
    TOP("ORA $1000,Y");
    TOP("ORA ($22,X)");
    TOP("ORA ($20),Y");
}

UTEST(m6502dasm, AND) {
    uint8_t prog[] = {
        0xA9, 0xFF,         // LDA #$FF
        0xA2, 0x01,         // LDX #$01
        0xA0, 0x02,         // LDY #$02
        0x29, 0xFF,         // AND #$FF
        0x25, 0x10,         // AND $10
        0x35, 0x10,         // AND $10,X
        0x2d, 0x00, 0x10,   // AND $1000
        0x3d, 0x00, 0x10,   // AND $1000,X
        0x39, 0x00, 0x10,   // AND $1000,Y
        0x21, 0x22,         // AND ($22,X)
        0x31, 0x20,         // AND ($20),Y
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$FF");
    TOP("LDX #$01");
    TOP("LDY #$02");
    TOP("AND #$FF");
    TOP("AND $10");
    TOP("AND $10,X");
    TOP("AND $1000");
    TOP("AND $1000,X");
    TOP("AND $1000,Y");
    TOP("AND ($22,X)");
    TOP("AND ($20),Y");
}

UTEST(m6502dasm, EOR) {
    uint8_t prog[] = {
        0xA9, 0xFF,         // LDA #$FF
        0xA2, 0x01,         // LDX #$01
        0xA0, 0x02,         // LDY #$02
        0x49, 0xFF,         // EOR #$FF
        0x45, 0x10,         // EOR $10
        0x55, 0x10,         // EOR $10,X
        0x4d, 0x00, 0x10,   // EOR $1000
        0x5d, 0x00, 0x10,   // EOR $1000,X
        0x59, 0x00, 0x10,   // EOR $1000,Y
        0x41, 0x22,         // EOR ($22,X)
        0x51, 0x20,         // EOR ($20),Y
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$FF");
    TOP("LDX #$01");
    TOP("LDY #$02");
    TOP("EOR #$FF");
    TOP("EOR $10");
    TOP("EOR $10,X");
    TOP("EOR $1000");
    TOP("EOR $1000,X");
    TOP("EOR $1000,Y");
    TOP("EOR ($22,X)");
    TOP("EOR ($20),Y");
}

UTEST(m6502dasm, NOP) {
    uint8_t prog[] = {
        0xEA,       // NOP
    };
    init(0, prog, sizeof(prog));
    TOP("NOP");
}

UTEST(m6502dasm, PHA_PLA_PHP_PLP) {
    uint8_t prog[] = {
        0xA9, 0x23,     // LDA #$23
        0x48,           // PHA
        0xA9, 0x32,     // LDA #$32
        0x68,           // PLA
        0x08,           // PHP
        0xA9, 0x00,     // LDA #$00
        0x28,           // PLP
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$23");
    TOP("PHA");
    TOP("LDA #$32");
    TOP("PLA");
    TOP("PHP");
    TOP("LDA #$00");
    TOP("PLP");
}

UTEST(m6502dasm, CLC_SEC_CLI_SEI_CLV_CLD_SED) {
    uint8_t prog[] = {
        0xB8,       // CLV
        0x78,       // SEI
        0x58,       // CLI
        0x38,       // SEC
        0x18,       // CLC
        0xF8,       // SED
        0xD8,       // CLD
    };
    init(0, prog, sizeof(prog));
    TOP("CLV");
    TOP("SEI");
    TOP("CLI");
    TOP("SEC");
    TOP("CLC");
    TOP("SED");
    TOP("CLD");
}

UTEST(m6502dasm, INC_DEC) {
    uint8_t prog[] = {
        0xA2, 0x10,         // LDX #$10
        0xE6, 0x33,         // INC $33
        0xF6, 0x33,         // INC $33,X
        0xEE, 0x00, 0x10,   // INC $1000
        0xFE, 0x00, 0x10,   // INC $1000,X
        0xC6, 0x33,         // DEC $33
        0xD6, 0x33,         // DEC $33,X
        0xCE, 0x00, 0x10,   // DEC $1000
        0xDE, 0x00, 0x10,   // DEC $1000,X
    };
    init(0, prog, sizeof(prog));
    TOP("LDX #$10");
    TOP("INC $33");
    TOP("INC $33,X");
    TOP("INC $1000");
    TOP("INC $1000,X");
    TOP("DEC $33");
    TOP("DEC $33,X");
    TOP("DEC $1000");
    TOP("DEC $1000,X");
}

UTEST(m6502dasm, ADC_SBC) {
    uint8_t prog[] = {
        0xA9, 0x01,         // LDA #$01
        0x85, 0x10,         // STA $10
        0x8D, 0x00, 0x10,   // STA $1000
        0xA9, 0xFC,         // LDA #$FC
        0xA2, 0x08,         // LDX #$08
        0xA0, 0x04,         // LDY #$04
        0x18,               // CLC
        0x69, 0x01,         // ADC #$01
        0x65, 0x10,         // ADC $10
        0x75, 0x08,         // ADC $8,X
        0x6D, 0x00, 0x10,   // ADC $1000
        0x7D, 0xF8, 0x0F,   // ADC $0FF8,X
        0x79, 0xFC, 0x0F,   // ADC $0FFC,Y
        // FIXME: ADC (zp,X) and ADC (zp),X
        0xF9, 0xFC, 0x0F,   // SBC $0FFC,Y
        0xFD, 0xF8, 0x0F,   // SBC $0FF8,X
        0xED, 0x00, 0x10,   // SBC $1000
        0xF5, 0x08,         // SBC $8,X
        0xE5, 0x10,         // SBC $10
        0xE9, 0x01,         // SBC #$01
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$01");
    TOP("STA $10");
    TOP("STA $1000");
    TOP("LDA #$FC");
    TOP("LDX #$08");
    TOP("LDY #$04");
    TOP("CLC");
    TOP("ADC #$01");
    TOP("ADC $10");
    TOP("ADC $08,X");
    TOP("ADC $1000");
    TOP("ADC $0FF8,X");
    TOP("ADC $0FFC,Y");
    TOP("SBC $0FFC,Y");
    TOP("SBC $0FF8,X");
    TOP("SBC $1000");
    TOP("SBC $08,X");
    TOP("SBC $10");
    TOP("SBC #$01");
}

UTEST(m6502dasm, CMP_CPX_CPY) {
    uint8_t prog[] = {
        0xA9, 0x01,     // LDA #$01
        0xA2, 0x02,     // LDX #$02
        0xA0, 0x03,     // LDY #$03
        0xC9, 0x00,     // CMP #$00
        0xC9, 0x01,     // CMP #$01
        0xC9, 0x02,     // CMP #$02
        0xE0, 0x01,     // CPX #$01
        0xE0, 0x02,     // CPX #$02
        0xE0, 0x03,     // CPX #$03
        0xC0, 0x02,     // CPY #$02
        0xC0, 0x02,     // CPY #$02
        0xC0, 0x03,     // CPY #$03
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$01");
    TOP("LDX #$02");
    TOP("LDY #$03");
    TOP("CMP #$00");
    TOP("CMP #$01");
    TOP("CMP #$02");
    TOP("CPX #$01");
    TOP("CPX #$02");
    TOP("CPX #$03");
    TOP("CPY #$02");
    TOP("CPY #$02");
    TOP("CPY #$03");
}

UTEST(m6502dasm, ASL) {
    uint8_t prog[] = {
        0xA9, 0x81,     // LDA #$81
        0xA2, 0x01,     // LDX #$01
        0x85, 0x10,     // STA $10
        0x06, 0x10,     // ASL $10
        0x16, 0x0F,     // ASL $0F,X
        0x0A,           // ASL
        0x0A,           // ASL
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$81");
    TOP("LDX #$01");
    TOP("STA $10");
    TOP("ASL $10");
    TOP("ASL $0F,X");
    TOP("ASL");
    TOP("ASL");
}

UTEST(m6502dasm, LSR) {
    uint8_t prog[] = {
        0xA9, 0x81,     // LDA #$81
        0xA2, 0x01,     // LDX #$01
        0x85, 0x10,     // STA $10
        0x46, 0x10,     // LSR $10
        0x56, 0x0F,     // LSR $0F,X
        0x4A,           // LSR
        0x4A,           // LSR
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$81");
    TOP("LDX #$01");
    TOP("STA $10");
    TOP("LSR $10");
    TOP("LSR $0F,X");
    TOP("LSR");
    TOP("LSR");
}

UTEST(m6502dasm, ROR_ROL) {
    uint8_t prog[] = {
        0xA9, 0x81,     // LDA #$81
        0xA2, 0x01,     // LDX #$01
        0x85, 0x10,     // STA $10
        0x26, 0x10,     // ROL $10
        0x36, 0x0F,     // ROL $0F,X
        0x76, 0x0F,     // ROR $0F,X
        0x66, 0x10,     // ROR $10
        0x6A,           // ROR
        0x6A,           // ROR
        0x2A,           // ROL
        0x2A,           // ROL
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$81");
    TOP("LDX #$01");
    TOP("STA $10");
    TOP("ROL $10");
    TOP("ROL $0F,X");
    TOP("ROR $0F,X");
    TOP("ROR $10");
    TOP("ROR");
    TOP("ROR");
    TOP("ROL");
    TOP("ROL");
}

UTEST(m6502dasm, BIT) {
    uint8_t prog[] = {
        0xA9, 0x00,         // LDA #$00
        0x85, 0x1F,         // STA $1F
        0xA9, 0x80,         // LDA #$80
        0x85, 0x20,         // STA $20
        0xA9, 0xC0,         // LDA #$C0
        0x8D, 0x00, 0x10,   // STA $1000
        0x24, 0x1F,         // BIT $1F
        0x24, 0x20,         // BIT $20
        0x2C, 0x00, 0x10    // BIT $1000
    };
    init(0, prog, sizeof(prog));
    TOP("LDA #$00");
    TOP("STA $1F");
    TOP("LDA #$80");
    TOP("STA $20");
    TOP("LDA #$C0");
    TOP("STA $1000");
    TOP("BIT $1F");
    TOP("BIT $20");
    TOP("BIT $1000");
}

UTEST(m6502dasm, BNE_BEQ) {
    uint8_t prog[] = {
        0xA9, 0x10,         // LDA #$10
        0xC9, 0x10,         // CMP #$10
        0xF0, 0x02,         // BEQ eq
        0xA9, 0x0F,         // ne: LDA #$0F
        0xC9, 0x0F,         // eq: CMP #$0F
        0xD0, 0xFA,         // BNE ne -> executed 2x, second time not taken
        0xEA,
    };
    init(0x0200, prog, sizeof(prog));
    TOP("LDA #$10");
    TOP("CMP #$10");
    TOP("BEQ $0208");
    TOP("LDA #$0F");
    TOP("CMP #$0F");
    TOP("BNE $0206");
    TOP("NOP");
}

UTEST(m6502dasm, JMP) {
    uint8_t prog[] = {
        0x4C, 0x00, 0x10,   // JMP $1000
    };
    init(0x0200, prog, sizeof(prog));
    TOP("JMP $1000");
}

UTEST(m6502dasm, JMP_indirect_samepage) {
    uint8_t prog[] = {
        0xA9, 0x33,         // LDA #$33
        0x8D, 0x10, 0x21,   // STA $2110
        0xA9, 0x22,         // LDA #$22
        0x8D, 0x11, 0x21,   // STA $2111
        0x6C, 0x10, 0x21,   // JMP ($2110)
    };
    init(0x0200, prog, sizeof(prog));
    TOP("LDA #$33");
    TOP("STA $2110");
    TOP("LDA #$22");
    TOP("STA $2111");
    TOP("JMP ($2110)");
}

UTEST(m6502dasm, JMP_indirect_wrap) {
    uint8_t prog[] = {
        0xA9, 0x33,         // LDA #$33
        0x8D, 0xFF, 0x21,   // STA $21FF
        0xA9, 0x22,         // LDA #$22
        0x8D, 0x00, 0x21,   // STA $2100    // note: wraps around!
        0x6C, 0xFF, 0x21,   // JMP ($21FF)
    };
    init(0x0200, prog, sizeof(prog));
    TOP("LDA #$33");
    TOP("STA $21FF");
    TOP("LDA #$22");
    TOP("STA $2100");
    TOP("JMP ($21FF)");
}

UTEST(m6502dasm, JSR_RTS) {
    uint8_t prog[] = {
        0x20, 0x05, 0x03,   // JSR fun
        0xEA, 0xEA,         // NOP, NOP
        0xEA,               // fun: NOP
        0x60,               // RTS
    };
    init(0x0300, prog, sizeof(prog));
    TOP("JSR $0305");
    TOP("NOP");
    TOP("NOP");
    TOP("NOP");
    TOP("RTS");
}

UTEST(m6502dasm, RTI) {
    uint8_t prog[] = {
        0xA9, 0x11,     // LDA #$11
        0x48,           // PHA
        0xA9, 0x22,     // LDA #$22
        0x48,           // PHA
        0xA9, 0x33,     // LDA #$33
        0x48,           // PHA
        0x40,           // RTI
    };
    init(0x0200, prog, sizeof(prog));
    TOP("LDA #$11");
    TOP("PHA");
    TOP("LDA #$22");
    TOP("PHA");
    TOP("LDA #$33");
    TOP("PHA");
    TOP("RTI");
}
