//------------------------------------------------------------------------------
//  m6502-wltest.c
//  Runs the CPU-parts of the Wolfgang Lorenz C64 test suite
//  (see: http://6502.org/tools/emu/)
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/mem.h"
#include <stdio.h>
#include "testsuite-2.15/bin/dump.h"

m6502_t cpu;
mem_t mem;
uint8_t ram[1<<16];
bool text_enabled = true;

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

    /* init CPU registers */
    cpu.state.S = 0xFD;
    cpu.state.P = 0x04;
    cpu.state.PC = 0x0801;

    return true;
}

/* pop return address from CPU stack */
uint16_t pop() {
    cpu.state.S++;
    uint8_t l = mem_rd(&mem, 0x0100|cpu.state.S++);
    uint8_t h = mem_rd(&mem, 0x0100|cpu.state.S);
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
bool handle_trap() {
    if (cpu.trap_id == 1) {
        /* print character */
        mem_wr(&mem, 0x030C, 0x00);
        if (text_enabled) {
            putchar(petscii2ascii(cpu.state.A));
        }
        cpu.state.PC = pop();
        cpu.state.PC++;
    }
    else if (cpu.trap_id == 2) {
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
        cpu.state.PC = 0x0816;
        text_enabled = true;
    }
    else if (cpu.trap_id == 3) {
        /* scan keyboard, this is called when an error was encountered,
           we'll continue, but disable text output until the next test is loaded
        */
        if (text_enabled) {
            puts("\nSKIP TEXT OUTPUT UNTIL NEXT TEST\n");
        }
        text_enabled = false;
        cpu.state.A = 0x02;
        cpu.state.PC = pop();
        cpu.state.PC++;
    }
    else if ((cpu.state.PC == 0x8000) || (cpu.state.PC == 0xA474)) {
        /* done */
        return false;
    }
    return true;
}

uint64_t tick(uint64_t pins, void* user_data) {
    const uint16_t addr = M6502_GET_ADDR(pins);
    if (pins & M6502_RW) {
        /* memory read */
        M6502_SET_DATA(pins, mem_rd(&mem, addr));
    }
    else {
        /* memory write */
        mem_wr(&mem, addr, M6502_GET_DATA(pins));
    }
    return pins;
}

int trap(uint16_t pc, void* user_data) {
    switch (pc) {
        case 0xFFD2: return 1; /* trap for print character function */
        case 0xE16F: return 2; /* trap for load dump function */
        case 0xFFE4: return 3; /* trap for 'scan keyboard' function */
        case 0x8000: return 4; /* traps for error and finished */
        case 0xA474: return 5;
        default: return 0;
    }
}

int main() {
    puts(">>> Running Wolfgang Lorenz C64 test suite...");

    /* prepare environment (see http://www.softwolves.com/arkiv/cbm-hackers/7/7114.html) */
    memset(ram, 0, sizeof(ram));
    mem_map_ram(&mem, 0, 0x0000, sizeof(ram), ram);

    m6502_init(&cpu, &(m6502_desc_t){
        .tick_cb = tick
    });
    m6502_reset(&cpu);
    m6502_trap_cb(&cpu, trap, 0);

    load_test("_start");
    bool done = false;
    while (!done) {
        m6502_exec(&cpu, (1<<30));
        if (!handle_trap()) {
            done = true;
        }
    }
    return 0;
}


