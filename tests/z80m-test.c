//------------------------------------------------------------------------------
//  z80m-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/z80m.h"
#include <stdio.h>

z80m_t cpu;
uint8_t mem[1<<16] = { 0 };
uint16_t out_port = 0;
uint8_t out_byte = 0;
uint64_t tick(int num, uint64_t pins, void* user_data) {
    if (pins & Z80M_MREQ) {
        if (pins & Z80M_RD) {
            /* memory read */
            Z80M_SET_DATA(pins, mem[Z80M_GET_ADDR(pins)]);
        }
        else if (pins & Z80M_WR) {
            /* memory write */
            mem[Z80M_GET_ADDR(pins)] = Z80M_GET_DATA(pins);
        }
    }
    else if (pins & Z80M_IORQ) {
        if (pins & Z80M_RD) {
            /* IN */
            Z80M_SET_DATA(pins, (Z80M_GET_ADDR(pins) & 0xFF) * 2);
        }
        else if (pins & Z80M_WR) {
            /* OUT */
            out_port = Z80M_GET_ADDR(pins);
            out_byte = Z80M_GET_DATA(pins);
        }
    }
    return pins;
}

void init() {
    z80m_init(&cpu, &(z80m_desc_t) { .tick_cb = tick, });
    z80m_set_f(&cpu, 0);
    z80m_set_fa_(&cpu, 0xFF00);
}

void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    assert((addr + num) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num);
}

uint32_t step() {
    return z80m_exec(&cpu, 0);
}

bool flags(uint8_t expected) {
    /* don't check undocumented flags */
    return (z80m_f(&cpu) & ~(Z80M_YF|Z80M_XF)) == expected;
}

uint16_t mem16(uint16_t addr) {
    uint8_t l = mem[addr];
    uint8_t h = mem[addr+1];
    return (h<<8)|l;
}

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

void SET_GET() {
    puts(">>> SET GET registers");
    init();
    z80m_set_f(&cpu, 0x01); z80m_set_a(&cpu, 0x23);
    z80m_set_h(&cpu, 0x45); z80m_set_l(&cpu, 0x67);
    z80m_set_d(&cpu, 0x89); z80m_set_e(&cpu, 0xAB);
    z80m_set_b(&cpu, 0xCD); z80m_set_c(&cpu, 0xEF);
    T(0x01 == z80m_f(&cpu)); T(0x23 == z80m_a(&cpu)); T(0x0123 == z80m_fa(&cpu));
    T(0x45 == z80m_h(&cpu)); T(0x67 == z80m_l(&cpu)); T(0x4567 == z80m_hl(&cpu));
    T(0x89 == z80m_d(&cpu)); T(0xAB == z80m_e(&cpu)); T(0x89AB == z80m_de(&cpu));
    T(0xCD == z80m_b(&cpu)); T(0xEF == z80m_c(&cpu)); T(0xCDEF == z80m_bc(&cpu));

    z80m_set_fa(&cpu, 0x1234);
    z80m_set_hl(&cpu, 0x5678);
    z80m_set_de(&cpu, 0x9abc);
    z80m_set_bc(&cpu, 0xdef0);
    z80m_set_fa_(&cpu, 0x4321);
    z80m_set_hl_(&cpu, 0x8765);
    z80m_set_de_(&cpu, 0xCBA9);
    z80m_set_bc_(&cpu, 0x0FED);
    T(0x12 == z80m_f(&cpu)); T(0x34 == z80m_a(&cpu)); T(0x1234 == z80m_fa(&cpu));
    T(0x56 == z80m_h(&cpu)); T(0x78 == z80m_l(&cpu)); T(0x5678 == z80m_hl(&cpu));
    T(0x9A == z80m_d(&cpu)); T(0xBC == z80m_e(&cpu)); T(0x9ABC == z80m_de(&cpu));
    T(0xDE == z80m_b(&cpu)); T(0xF0 == z80m_c(&cpu)); T(0xDEF0 == z80m_bc(&cpu));
    T(0x4321 == z80m_fa_(&cpu));
    T(0x8765 == z80m_hl_(&cpu));
    T(0xCBA9 == z80m_de_(&cpu));
    T(0x0FED == z80m_bc_(&cpu));

    z80m_set_pc(&cpu, 0xCEDF);
    z80m_set_wz(&cpu, 0x1324);
    z80m_set_sp(&cpu, 0x2435);
    z80m_set_i(&cpu, 0x35);
    z80m_set_r(&cpu, 0x46);
    z80m_set_ix(&cpu, 0x4657);
    z80m_set_iy(&cpu, 0x5768);
    T(0xCEDF == z80m_pc(&cpu));
    T(0x1324 == z80m_wz(&cpu));
    T(0x2435 == z80m_sp(&cpu));
    T(0x35 == z80m_i(&cpu));
    T(0x46 == z80m_r(&cpu));
    T(0x4657 == z80m_ix(&cpu));
    T(0x5768 == z80m_iy(&cpu));

    z80m_set_im(&cpu, 2);
    z80m_set_iff1(&cpu, true);
    z80m_set_iff2(&cpu, false);
    z80m_set_ei_pending(&cpu, true);
    T(2 == z80m_im(&cpu));
    T(z80m_iff1(&cpu));
    T(!z80m_iff2(&cpu));
    T(z80m_ei_pending(&cpu));
    z80m_set_iff1(&cpu, false);
    z80m_set_iff2(&cpu, true);
    z80m_set_ei_pending(&cpu, false);
    T(!z80m_iff1(&cpu));
    T(z80m_iff2(&cpu));
    T(!z80m_ei_pending(&cpu));
}

void EXEC() {
    init();
    z80m_exec(&cpu, 0);
}

int main() {
    EXEC();
    SET_GET();
    printf("%d tests run ok.\n", num_tests);
    return 0;
}

