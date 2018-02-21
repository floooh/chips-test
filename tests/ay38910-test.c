//------------------------------------------------------------------------------
//  ay38910-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/ay38910.h"

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }
#define PINS(p,d) ((p)|((d&0xFF)<<16))

/* pin mask to select register address */
#define ADDR(d) PINS(AY38910_BDIR|AY38910_BC1,d)
/* pin mask to write value to register */
#define WRITE(d) PINS(AY38910_BDIR,d)
/* pin mask to read value from register */
#define READ() PINS(AY38910_BC1,0)
/* extract data from pin mask */
#define DATA(p) ((p>>16)&0xFF)

void test_read_write() {
    ay38910_t ay;
    ay38910_desc_t ay_desc = {
        .type = AY38910_TYPE_8912,
        .tick_hz = 1000*1000,
        .sound_hz = 44100,
        .magnitude = 1.0f
    };
    ay38910_init(&ay, &ay_desc);

    /* each register write is 2 ops, first the register address, then the value */
    ay38910_iorq(&ay, ADDR(AY38910_REG_PERIOD_B_FINE));
    T(ay.addr == 2);
    ay38910_iorq(&ay, WRITE(0x7f));
    T(ay.reg[AY38910_REG_PERIOD_B_FINE] == 0x7f);
    T(ay.tone[1].period == 0x7f);
    ay38910_iorq(&ay, ADDR(AY38910_REG_PERIOD_B_COARSE));
    T(ay.addr == 3);
    ay38910_iorq(&ay, WRITE(0x12)); /* only 4 bits should be valid for coarse period */
    T(ay.reg[AY38910_REG_PERIOD_B_COARSE] == 0x02);
    T(ay.tone[1].period == 0x27f);

    /* read the just written registers */
    uint64_t pins;
    ay38910_iorq(&ay, ADDR(AY38910_REG_PERIOD_B_FINE));
    pins = ay38910_iorq(&ay, READ());
    T(DATA(pins) == 0x7f);
    ay38910_iorq(&ay, ADDR(AY38910_REG_PERIOD_B_COARSE));
    pins = ay38910_iorq(&ay, READ());
    T(DATA(pins) == 0x02);
}

int main() {
    test_read_write();
    return 0;
}
