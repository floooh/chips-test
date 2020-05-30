//------------------------------------------------------------------------------
//  z80-int.c
//------------------------------------------------------------------------------
#include "chips/z80.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

#define _A z80_a(&cpu)
#define _F z80_f(&cpu)
#define _L z80_l(&cpu)
#define _H z80_h(&cpu)
#define _E z80_e(&cpu)
#define _D z80_d(&cpu)
#define _C z80_c(&cpu)
#define _B z80_b(&cpu)
#define _FA z80_fa(&cpu)
#define _HL z80_hl(&cpu)
#define _DE z80_de(&cpu)
#define _BC z80_bc(&cpu)
#define _FA_ z80_fa_(&cpu)
#define _HL_ z80_hl_(&cpu)
#define _DE_ z80_de_(&cpu)
#define _BC_ z80_bc_(&cpu)
#define _PC z80_pc(&cpu)
#define _SP z80_sp(&cpu)
#define _WZ z80_wz(&cpu)
#define _IR z80_ir(&cpu)
#define _I z80_i(&cpu)
#define _R z80_r(&cpu)
#define _IX z80_ix(&cpu)
#define _IY z80_iy(&cpu)
#define _IM z80_im(&cpu)
#define _IFF1 z80_iff1(&cpu)
#define _IFF2 z80_iff2(&cpu)
#define _EI_PENDING z80_ei_pending(&cpu)

static z80_t cpu;
static uint8_t mem[1<<16] = { 0 };
static bool reti_executed = false;

static uint64_t tick(int num, uint64_t pins, void* user_data) {
    (void)num;
    (void)user_data;
    if (pins & Z80_MREQ) {
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            mem[addr] = Z80_GET_DATA(pins);
        }
    }
    else if (pins & Z80_IORQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, 0xFF);
        }
        else if (pins & Z80_WR) {
            /* request interrupt when a IORQ|WR happens */
            pins |= Z80_INT;
        }
        else if (pins & Z80_M1) {
            /* an interrupt acknowledge cycle, need to provide interrupt vector */
            Z80_SET_DATA(pins, 0xE0);
        }
    }
    if (pins & Z80_RETI) {
        /* reti was executed */
        reti_executed = true;
        pins &= ~Z80_RETI;
    }
    return pins;
}

static void w16(uint16_t addr, uint16_t data) {
    mem[addr]   = (uint8_t) data;
    mem[addr+1] = (uint8_t) (data>>8);
}

static void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    if ((addr + num) <= sizeof(mem)) {
        memcpy(&mem[addr], bytes, num);
    }
}

UTEST(z80int, z80int) {
    z80_init(&cpu, &(z80_desc_t){ .tick_cb = tick });

    /* place the address 0x0200 of an interrupt service routine at 0x00E0 */
    w16(0x00E0, 0x0200);

    /* the main program load I with 0, and execute an OUT,
       which triggeres an interrupt, afterwards load some
       value into HL
    */
    uint8_t prog[] = {
        0x31, 0x00, 0x03,   /* LD SP,0x0300 */
        0xFB,               /* EI */
        0xED, 0x5E,         /* IM 2 */
        0xAF,               /* XOR A */
        0xED, 0x47,         /* LD I,A */
        0xD3, 0x01,         /* OUT (0x01),A -> this should request an interrupt */
        0x21, 0x33, 0x33,   /* LD HL,0x3333 */
    };
    copy(0x0100, prog, sizeof(prog));
    z80_set_pc(&cpu, 0x0100);

    /* the interrupt service routine */
    uint8_t int_prog[] = {
        0xFB,               /* EI */
        0x21, 0x11, 0x11,   /* LD HL,0x1111 */
        0xED, 0x4D,         /* RETI */
    };
    copy(0x0200, int_prog, sizeof(int_prog));

    T(10 == z80_exec(&cpu,0)); T(_SP == 0x0300);   /* LD SP, 0x0300) */
    T(4 == z80_exec(&cpu,0)); T(_IFF1);                       /* EI */
    T(8 == z80_exec(&cpu,0)); T(_IFF2); T(_IM == 2);        /* IM 2 */
    T(4 == z80_exec(&cpu,0)); T(_A == 0);                              /* XOR A */
    T(9 == z80_exec(&cpu,0)); T(_I == 0);                              /* LD I,A */
    T(29 == z80_exec(&cpu,0)); T(_PC == 0x0200); T(_IFF2 == false); T(_SP == 0x02FE);    /* OUT (0x01),A */
    T(4 == z80_exec(&cpu,0)); T(_IFF2);                       /* EI */
    T(10 == z80_exec(&cpu,0)); T(_HL == 0x1111); T(_IFF2 == true);  /* LD HL,0x1111 */
    T(14 == z80_exec(&cpu,0)); T(_PC == 0x010B); T(reti_executed);     /* RETI */
    z80_exec(&cpu,0); T(_HL == 0x3333); /* LD HL,0x3333 */
}

