//------------------------------------------------------------------------------
//  c64-ciatest.c
//
//  Runs the VICE 'cia-timer' test program (tests/vice-tests/CIA/cia-timer) on
//  the chips C64 emulation and reports which of its eight sub-tests fail.
//
//      c64-ciatest tests/vice-tests/CIA/cia-timer/cia-timer-oldcias.prg
//
//  The test program is self-checking: it runs eight timer/ICR measurements,
//  writes the results into screen RAM, and finally compares them against a
//  reference dump that's assembled into the .prg ('dump-oldcia.bin' for the
//  'oldcias' build). For every compared cell it writes green (5) or red (2)
//  into color RAM, and the border ends up green if everything matched.
//
//  The catch: the check *overwrites* every mismatching screen cell with the
//  expected value, so by the time the result is visible the measured values
//  are gone. This tool therefore ticks the machine one cycle at a time and
//  snapshots screen RAM the instant the check writes its first color cell,
//  which is what makes a failure diagnosable ("measured $13, expected $12")
//  instead of just red.
//
//  The eight sub-tests are CIA1/CIA2 x timer A/B x interrupt disabled/enabled.
//  The four CIA1 blocks run their handler off IRQ, the four CIA2 blocks off
//  NMI - the interrupt only actually fires in the ICR=1 blocks, where the last
//  two rows of a block are written by the handler.
//
//  The reference dump is looked up next to the .prg, or given with --ref=FILE.
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c1530.h"
#include "chips/m6522.h"
#include "systems/c1541.h"
#include "systems/c64.h"
#include "c64-roms.h"

#define SLICE_USEC (20000)      // emulate in ~50Hz slices
#define DEF_BOOT_FRAMES (180)   // frames to let the C64 boot before quickloading
#define MAX_RUN_TICKS (30*1000*1000)    // give up if the test never reaches its self-check

#define SCREEN_ADDR (0x0400)
#define CHECK_FIRST (0x28)      // first screen offset the test compares
#define CHECK_SIZE  (960)       // ...and how many cells

// the eight sub-tests, in the order the test program runs them. The screen
// addresses are the 'out' table in cia-timer.asm, the register values are the
// 'icr', 'cr', 'tlow' and 'cianr' tables.
typedef struct {
    const char* name;
    uint16_t addr;              // screen address of the block's first row
    bool nmi;                   // CIA2 blocks pull NMI instead of IRQ
    bool irq_enabled;           // ICR mask enables the timer interrupt
} block_t;

static const block_t blocks[8] = {
    { "CIA1 TA ICR=0", 0x0450, false, false },
    { "CIA1 TA ICR=1", 0x0518, false, true  },
    { "CIA1 TB ICR=0", 0x0464, false, false },
    { "CIA1 TB ICR=1", 0x052C, false, true  },
    { "CIA2 TA ICR=0", 0x0608, true,  false },
    { "CIA2 TA ICR=1", 0x06D0, true,  true  },
    { "CIA2 TB ICR=0", 0x061C, true,  false },
    { "CIA2 TB ICR=1", 0x06E4, true,  true  },
};

// what the five rows of a block hold, see cia-timer.asm
static const char* row_names[5] = {
    "timer lo   ",      // output0: timer low byte, minus $15
    "ICR read 1 ",      // output1: first ICR read after the underflow
    "ICR read 2 ",      // output2: second ICR read
    "ICR in ISR ",      // output3: ICR as seen by the interrupt handler
    "timer in ISR",     // output4: timer low byte as seen by the handler
};

static c64_t c64;
static uint8_t screen[1024];    // screen RAM snapshot taken at the first color RAM write of the self-check

static uint8_t* load_file(const char* path, size_t* out_size) {
    *out_size = 0;
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t* ptr = 0;
    if (size > 0) {
        ptr = malloc((size_t)size);
        if (fread(ptr, 1, (size_t)size, fp) == (size_t)size) {
            *out_size = (size_t)size;
        }
        else {
            free(ptr);
            ptr = 0;
        }
    }
    fclose(fp);
    return ptr;
}

// 'cia-timer-oldcias.prg' -> '<dir>/dump-oldcia.bin'
static void make_ref_path(const char* prg_path, char* out, size_t out_size) {
    const char* slash = strrchr(prg_path, '/');
    const char* name = slash ? slash + 1 : prg_path;
    const int dir_len = (int)(name - prg_path);
    const char* which = strstr(name, "new") ? "new" : "old";
    snprintf(out, out_size, "%.*sdump-%scia.bin", dir_len, prg_path, which);
}

// the test program's final self-check is the only thing that ever writes red
// (2) or green (5) into color RAM, so color RAM doubles as the 'test has
// finished' flag (it clears color RAM to 1 on startup)
static bool check_has_run(void) {
    const uint8_t c = c64.color_ram[CHECK_FIRST];
    return (c == 2) || (c == 5);
}

// boot, quickload the test, and run it until the self-check has happened
static bool run_program(const char* path, int boot_frames) {
    size_t size = 0;
    uint8_t* data = load_file(path, &size);
    if (!data) {
        fprintf(stderr, "cannot load '%s'\n", path);
        return false;
    }
    memset(&c64, 0, sizeof(c64));
    c64_init(&c64, &(c64_desc_t){
        .roms = {
            .chars = { .ptr=dump_c64_char_bin, .size=sizeof(dump_c64_char_bin) },
            .basic = { .ptr=dump_c64_basic_bin, .size=sizeof(dump_c64_basic_bin) },
            .kernal = { .ptr=dump_c64_kernalv3_bin, .size=sizeof(dump_c64_kernalv3_bin) }
        }
    });
    for (int i = 0; i < boot_frames; i++) {
        c64_exec(&c64, SLICE_USEC);
    }
    const bool loaded = c64_quickload(&c64, (chips_range_t){ .ptr = data, .size = size });
    free(data);
    if (!loaded) {
        fprintf(stderr, "cannot quickload '%s'\n", path);
        return false;
    }
    c64_basic_run(&c64);
    // Tick-accurate from here on: the self-check writes the color RAM cell
    // before it overwrites the matching screen cell, so stopping on the very
    // first color write leaves screen RAM fully intact - one 20ms slice of
    // slack would already lose the last sub-test, which is written only a few
    // dozen cycles before the check starts.
    for (uint32_t i = 0; i < MAX_RUN_TICKS; i++) {
        c64.pins = _c64_tick(&c64, c64.pins);
        if (check_has_run()) {
            memcpy(screen, &c64.ram[SCREEN_ADDR], sizeof(screen));
            // let the check finish so its own verdict can be read back
            for (int f = 0; f < 10; f++) {
                c64_exec(&c64, SLICE_USEC);
            }
            return true;
        }
    }
    fprintf(stderr, "test did not reach its self-check within %u ticks\n", MAX_RUN_TICKS);
    return false;
}

int main(int argc, char* argv[]) {
    const char* prg_path = 0;
    const char* ref_path = 0;
    int boot_frames = DEF_BOOT_FRAMES;
    bool verbose = false;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (0 == strncmp(arg, "--ref=", 6)) {
            ref_path = arg + 6;
        }
        else if (0 == strncmp(arg, "--boot=", 7)) {
            boot_frames = atoi(arg + 7);
        }
        else if (0 == strcmp(arg, "--verbose")) {
            verbose = true;    // also dump the blocks that pass
        }
        else if (arg[0] == '-') {
            fprintf(stderr, "usage: c64-ciatest [--ref=FILE] [--boot=N] [--verbose] <cia-timer-*.prg>\n");
            return 2;
        }
        else {
            prg_path = arg;
        }
    }
    if (!prg_path) {
        fprintf(stderr, "usage: c64-ciatest [--ref=FILE] [--boot=N] [--verbose] <cia-timer-*.prg>\n");
        return 2;
    }

    char ref_buf[1024];
    if (!ref_path) {
        make_ref_path(prg_path, ref_buf, sizeof(ref_buf));
        ref_path = ref_buf;
    }
    size_t ref_size = 0;
    uint8_t* ref = load_file(ref_path, &ref_size);
    if (!ref || (ref_size != CHECK_SIZE)) {
        fprintf(stderr, "cannot load reference '%s' (expected %d bytes)\n", ref_path, CHECK_SIZE);
        return 2;
    }

    if (!run_program(prg_path, boot_frames)) {
        return 2;
    }

    printf("%s (reference %s)\n\n", prg_path, ref_path);
    int total_diff = 0;
    for (int b = 0; b < 8; b++) {
        // count the mismatching cells of this block first, a block is 5 rows
        // of 16 cells at <addr>+1 .. <addr>+16
        int num_diff = 0;
        for (int row = 0; row < 5; row++) {
            for (int x = 1; x <= 16; x++) {
                const int ofs = (blocks[b].addr + row * 40 + x) - SCREEN_ADDR;
                if (screen[ofs] != ref[ofs - CHECK_FIRST]) {
                    num_diff++;
                }
            }
        }
        total_diff += num_diff;
        printf("[%s] test %d: %s%s", num_diff ? "FAIL" : "PASS", b, blocks[b].name,
            blocks[b].irq_enabled ? (blocks[b].nmi ? " (NMI)" : " (IRQ)") : "");
        if (num_diff) {
            printf(" - %d of 80 cells differ", num_diff);
        }
        putchar('\n');
        if (!num_diff && !verbose) {
            continue;
        }
        // dump measured vs expected, one line pair per row, x counts down from
        // 16 to 1 in the test so the leftmost cell is the last one measured
        for (int row = 0; row < 5; row++) {
            const int base = (blocks[b].addr + row * 40) - SCREEN_ADDR;
            int row_diff = 0;
            for (int x = 1; x <= 16; x++) {
                if (screen[base + x] != ref[base + x - CHECK_FIRST]) {
                    row_diff++;
                }
            }
            if (!row_diff && !verbose) {
                continue;
            }
            printf("    %s got:", row_names[row]);
            for (int x = 1; x <= 16; x++) {
                printf(" %02X", screen[base + x]);
            }
            printf("\n    %s exp:", "            ");
            for (int x = 1; x <= 16; x++) {
                printf(" %02X", ref[base + x - CHECK_FIRST]);
            }
            printf("\n    %s    ", "            ");
            for (int x = 1; x <= 16; x++) {
                printf(" %s", (screen[base + x] != ref[base + x - CHECK_FIRST]) ? "^^" : "  ");
            }
            putchar('\n');
        }
        putchar('\n');
    }

    // cross-check against the test program's own verdict, it should agree
    int prg_diff = 0;
    for (int i = 0; i < CHECK_SIZE; i++) {
        if (c64.color_ram[CHECK_FIRST + i] == 2) {
            prg_diff++;
        }
    }
    printf("\n%d of %d compared cells differ (test program itself flagged %d)\n",
        total_diff, CHECK_SIZE, prg_diff);
    free(ref);
    return (total_diff == 0) ? 0 : 1;
}
