//------------------------------------------------------------------------------
//  m6502-wltest.c
//  Runs the CPU-parts of the Wolfgang Lorenz C64 test suite
//  (see: http://6502.org/tools/emu/)
//------------------------------------------------------------------------------
// force assert() enabled
#define SOKOL_IMPL
#include "sokol_time.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/mem.h"
#include <stdio.h>
#include <inttypes.h>
#include "testsuite-2.15/bin/dump.h"
#ifdef NDEBUG
#undef NDEBUG
#endif

uint64_t cpu_pins;
m6502_t cpu;
mem_t mem;
uint8_t ram[1<<16];
bool text_enabled = true;

/* set CPU state to continue running at a specific address */
void cpu_goto(uint16_t addr) {
    M6502_SET_ADDR(cpu_pins, addr);
    M6502_SET_DATA(cpu_pins, mem_rd(&mem, addr));
    cpu_pins |= M6502_SYNC|M6502_RW;
    cpu.PC = addr;
}

/* load a test dump into memory, return false if last test is reached ('trap17') */
bool load_test(const char* name) {
    if (0 == strcmp(name, "trap1")) {
        /* last test reached */
        return false;
    }
    else if (0 == strcmp(name, "sbcb(eb)")) {
        name = "sbcb_eb";
    }
    const uint8_t* ptr = 0;
    int size = 0;
    for (int i = 0; i < DUMP_NUM_ITEMS; i++) {
        if (0 == strcmp(dump_items[i].name, name)) {
            ptr = dump_items[i].ptr;
            size = dump_items[i].size;
            break;
        }
    }
    assert(ptr && (size > 2));

    /* first 2 bytes of the dump are the start address */
    size -= 2;
    uint8_t l = *ptr++;
    uint8_t h = *ptr++;
    uint16_t addr = (h<<8)|l;
    mem_write_range(&mem, addr, ptr, size);

    /* initialize some memory locations */
    mem_wr(&mem, 0x0002, 0x00);
    mem_wr(&mem, 0xA002, 0x00);
    mem_wr(&mem, 0xA003, 0x80);
    mem_wr(&mem, 0xFFFE, 0x48);
    mem_wr(&mem, 0xFFFF, 0xFF);
    mem_wr(&mem, 0x01FE, 0xFF);
    mem_wr(&mem, 0x01FF, 0x7F);

    /* KERNAL IRQ handler at 0xFF48 */
    uint8_t irq_handler[] = {
        0x48,               // PHA
        0x8A,               // TXA
        0x48,               // PHA
        0x98,               // TYA
        0x48,               // PHA
        0xBA,               // TSX
        0xBD, 0x04, 0x01,   // LDA $0104,X
        0x29, 0x10,         // AND #$10
        0xF0, 0x03,         // BEQ $FF58
        0x6C, 0x16, 0x03,   // JMP ($0316)
        0x6C, 0x14, 0x03,   // JMP ($0314)
    };
    mem_write_range(&mem, 0xFF48, irq_handler, sizeof(irq_handler));

    /* continue execution at start address */
    cpu.S = 0xFD;
    cpu.P = M6502_BF|M6502_IF;
    cpu_goto(0x801);
    return true;
}

/* pop return address from CPU stack */
uint16_t pop() {
    cpu.S++;
    uint8_t l = mem_rd(&mem, 0x0100|cpu.S++);
    uint8_t h = mem_rd(&mem, 0x0100|cpu.S);
    uint16_t addr = (h<<8)|l;
    return addr;
}

/* hacky PETSCII to ASCII conversion */
char petscii2ascii(uint8_t p) {
    if (p < 0x20) {
        if (p == 0x0D) {
            return '\n';
        }
        else {
            return ' ';
        }
    }
    else if ((p >= 0x41) && (p <= 0x5A)) {
        return 'a' + (p-0x41);
    }
    else if ((p >= 0xC1) && (p <= 0xDA)) {
        return 'A' + (p-0xC1);
    }
    else if (p < 0x80) {
        return p;
    }
    else {
        return ' ';
    }
}

/* check for special trap addresses, and perform OS functions, return false to exit */
bool handle_trap(int trap_id) {
    if (trap_id == 1) {
        /* print character */
        mem_wr(&mem, 0x030C, 0x00);
        if (text_enabled) {
            putchar(petscii2ascii(cpu.A));
        }
        cpu_goto(pop() + 1);
    }
    else if (trap_id == 2) {
        /* load dump */
        uint8_t l = mem_rd(&mem, 0x00BB);   // petscii filename address, low byte
        uint8_t h = mem_rd(&mem, 0x00BC);   // petscii filename address, high byte
        uint16_t addr = (h<<8)|l;
        int s = mem_rd(&mem, 0x00B7);   // petscii filename length
        char name[64];
        for (int i = 0; i < s; i++) {
            name[i] = petscii2ascii(mem_rd(&mem, addr++));
        }
        name[s] = 0;
        if (!load_test(name)) {
            /* last test reached */
            return false;
        }
        pop();
        cpu_goto(0x0816);
        text_enabled = true;
    }
    else if (trap_id == 3) {
        /* scan keyboard, this is called when an error was encountered,
           we'll continue, but disable text output until the next test is loaded
        */
        if (text_enabled) {
            puts("\nSKIP TEXT OUTPUT UNTIL NEXT TEST\n");
        }
        text_enabled = false;
        cpu.A = 0x02;
        cpu_goto(pop() + 1);
    }
    else if ((cpu.PC == 0x8001) || (cpu.PC == 0xA475)) {
        /* done */
        return false;
    }
    return true;
}

static bool test_trap(uint16_t pc) {
    return ((cpu_pins & (M6502_SYNC|0xFFFF)) == (M6502_SYNC|pc));
}

int test_traps(void) {
    static const uint16_t traps[] = { 0xFFD2, 0xE16F, 0xFFE4, 0x8000, 0xA474 };
    for (int i = 0; i < (int)(sizeof(traps)/sizeof(uint16_t)); i++) {
        if (test_trap(traps[i])) {
            return i + 1;
        }
    }
    return 0;
}

void tick(void) {
    cpu_pins = m6502_tick(&cpu, cpu_pins);
    const uint16_t addr = M6502_GET_ADDR(cpu_pins);
    if (cpu_pins & M6502_RW) {
        /* memory read */
        M6502_SET_DATA(cpu_pins, mem_rd(&mem, addr));
    }
    else {
        /* memory write */
        mem_wr(&mem, addr, M6502_GET_DATA(cpu_pins));
    }
}

int main() {
    puts(">>> Running Wolfgang Lorenz C64 test suite...");
    stm_setup();

    /* prepare environment (see http://www.softwolves.com/arkiv/cbm-hackers/7/7114.html) */
    memset(ram, 0, sizeof(ram));
    mem_map_ram(&mem, 0, 0x0000, sizeof(ram), ram);

    /* init CPU and run through the reset sequence */
    m6502_desc_t desc;
    memset(&desc, 0, sizeof(desc));
    cpu_pins = m6502_init(&cpu, &desc);
    for (int i = 0; i < 7; i++) {
        tick();
    }

    /* run the tests */
    load_test("_start");
    bool done = false;
    uint64_t start_time = stm_now();
    uint64_t ticks = 0;
    while (!done) {
        tick();
        ticks++;
        if (cpu_pins & M6502_SYNC) {
            int trap_id = test_traps();
            if (0 != trap_id) {
                if (!handle_trap(trap_id)) {
                    done = true;
                }
            }
        }
    }
    double dur = stm_sec(stm_since(start_time));
    printf("\n%"PRIu64" cycles in %.3fsecs (%.2f MHz)\n", ticks, dur, (ticks/dur)/1000000.0);
    putchar('\n');
    return 0;
}


