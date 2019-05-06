//------------------------------------------------------------------------------
//  mem-test.c
//  Test mem.h features.
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/mem.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

UTEST(mem, mem) {
    mem_t mem;
    mem_init(&mem);

    /* initial state should be "everything is unmapped */
    T(0xFF == mem_rd(&mem, 0x0000));
    T(0xFF == mem_rd(&mem, 0xFFFF));
    T(0xFF == mem_rd(&mem, 0x1234));
    mem_wr(&mem, 0x2345, 0x12);
    T(0xFF == mem_rd(&mem, 0x2345));

    /* test stacked memory banks, where higher priority banks overlay lower priority banks */
    uint8_t bank3[1<<16]; memset(bank3, 0x12, sizeof(bank3));   // 64 KB
    uint8_t bank2[1<<15]; memset(bank2, 0x34, sizeof(bank2));   // 32 KB
    uint8_t bank1[1<<14]; memset(bank1, 0x56, sizeof(bank1));   // 16 KB
    uint8_t bank0[1<<13]; memset(bank0, 0x78, sizeof(bank0));   // 8 KB
    mem_map_ram(&mem, 3, 0x0000, sizeof(bank3), bank3);
    mem_map_ram(&mem, 2, 0x8000, sizeof(bank2), bank2);
    mem_map_ram(&mem, 1, 0xC000, sizeof(bank1), bank1);
    mem_map_ram(&mem, 0, 0xE000, sizeof(bank0), bank0);
    T(0x12 == mem_rd(&mem, 0x0000));
    T(0x12 == mem_rd(&mem, 0x7FFF));
    T(0x34 == mem_rd(&mem, 0x8000));
    T(0x34 == mem_rd(&mem, 0xBFFF));
    T(0x56 == mem_rd(&mem, 0xC000));
    T(0x56 == mem_rd(&mem, 0xDFFF));
    T(0x78 == mem_rd(&mem, 0xE000));
    T(0x78 == mem_rd(&mem, 0xFFFF));
    T(mem_readptr(&mem, 0x1000) == &bank3[0x1000]);
    T(mem_readptr(&mem, 0x9000) == &bank2[0x1000]);
    mem_wr(&mem, 0x1000, 0x33);
    mem_wr(&mem, 0x9000, 0x44);
    mem_wr(&mem, 0xD000, 0x55);
    mem_wr(&mem, 0xF000, 0x66);
    T(mem_rd(&mem, 0x1000) == 0x33);
    T(mem_rd(&mem, 0x9000) == 0x44);
    T(mem_rd(&mem, 0xD000) == 0x55);
    T(mem_rd(&mem, 0xF000) == 0x66);
    T(bank3[0x1000] == 0x33);
    T(bank2[0x1000] == 0x44);
    T(bank1[0x1000] == 0x55);
    T(bank0[0x1000] == 0x66);

    /* test ROM mapping */
    mem_map_rom(&mem, 0, 0xE000, sizeof(bank0), bank0);
    T(mem_rd(&mem, 0xF001) == 0x78);
    mem_wr(&mem, 0xF001, 0x77);
    T(mem_rd(&mem, 0xF001) == 0x78);

    /* different read/write areas (for RAM-behind-ROM) */
    uint8_t bank4[sizeof(bank0)]; memset(bank4, 0x9A, sizeof(bank4));   // 8 KByte
    mem_map_rw(&mem, 0, 0xE000, sizeof(bank0), bank0, bank4);
    T(mem_rd(&mem, 0xF800)==0x78);
    mem_wr(&mem, 0xF800, 0xAA);
    T(mem_rd(&mem, 0xF800)==0x78);
    T(bank4[0x1800] == 0xAA);
}


