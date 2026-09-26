//------------------------------------------------------------------------------
//  c64-icr2test.c
//
//  Runs the VICE 'cia-icr-test2' programs (tests/vice-tests/CIA/shiftregister)
//  on the chips C64 emulation and reports which measurements differ.
//
//      c64-icr2test tests/vice-tests/CIA/shiftregister/cia-icr-test2-oneshot.prg
//
//  The test starts CIA1 timer A with a latch of $0000..$0027 (one value per
//  column), enables the timer A interrupt with a write of $81 to $DC0D, then
//  disables all interrupts again with a write of $7F a few cycles later and
//  reads $DC0D. Clearing the mask must not clear the latched flag bits, so
//  whether the read returns $81, $01 or $00 pins down exactly when the
//  underflow, the interrupt and the two mask writes happen relative to each
//  other.
//
//  Three such loops are run, each with one more cycle between the register
//  writes, and their 40 results are stored in screen rows 0, 8 and 16. The
//  test program compares them against a reference dump assembled into the .prg
//  ('icr2-oneshot.ref', used for both the oneshot and the continues build),
//  colors every cell green or red, and writes $00 (ok) or $FF (failed) to
//  $D7FF.
//
//  Unlike the cia-timer test the self-check leaves screen RAM alone, but the
//  whole test loops forever, so this tool snapshots the screen at the first
//  write to $D7FF (which is a SID mirror and never lands in RAM - watch the
//  bus) and does its own comparison, to show measured vs expected per cell.
//
//  Exactly one cell per loop is expected to differ (see known_diff[] below),
//  so the exit code is a comparison against that list, not the test program's
//  own verdict.
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
#define RESULT_ADDR (0xD7FF)    // test writes $00 (ok) or $FF (failed) here
#define REF_SKIP (2)            // the .ref files start with a load address
#define NUM_COLS (40)           // one column per timer A latch value
#define TRACE_CYCLES (50)       // --trace: cycles to dump after the timer is started

// the three measurement loops, see cia-icr-test2.asm
typedef struct {
    int row;                    // screen row the results are stored in
    const char* name;
} loop_t;

static const loop_t loops[3] = {
    {  0, "sta $dc0d($81) / sta $dc0d($7f)          " },
    {  8, "sta $dc0d($81) / nop / sta $dc0d($7f)    " },
    { 16, "nop / sta $dc0d($81) / nop / sta $dc0d($7f)" },
};

/*  Cells which are expected to differ: the one column per loop where the $7F
    write lands exactly in the timer A underflow cycle.

    The reference says $81 (the interrupt happened, bit 7 latched), chips says
    $01 (the mask write killed the interrupt while it was still in the 1-cycle
    delay pipeline). That difference is the old-vs-new CIA behaviour, and chips
    emulates the old CIA:

    - the sibling tests in this directory come in old/new pairs, and row 8 of
      icr-oneshot-old.ref is '0101..01' where icr-oneshot-new.ref is '8181..81'
      - i.e. the disputed value *is* the CIA revision difference. Running
      cia-icr-test-{oneshot,continues}-old.prg with --ref=icr-{...}-old.ref
      gives 0 of 120 differing cells, the -new.prg/-new.ref pair gives 16.
    - Wilfred Bos' dd0dtest (tests/vice-tests/CIA/dd0dtest) detects the CIA
      revision itself, prints 'OLD CIA DETECTED' here, and its test 11 demands
      the old-CIA direction for this exact cycle.

    cia-icr-test2 only ships a single reference and it encodes the new-CIA
    behaviour at the boundary, so these three cells can't be satisfied at the
    same time as dd0dtest test 11.
*/
static const struct { int loop; int col; } known_diff[] = {
    { 0, 0x09 },
    { 1, 0x0B },
    { 2, 0x0D },
};
#define NUM_KNOWN_DIFF (int)(sizeof(known_diff) / sizeof(known_diff[0]))

/*  ...but only when comparing against the test's own reference - with --ref=
    this tool also runs the sibling cia-icr-test-*-old/new.prg programs, and
    those have their own (matching) references.
*/
static bool check_known_diff = true;

static bool is_known_diff(int loop, int col) {
    if (!check_known_diff) {
        return false;
    }
    for (int i = 0; i < NUM_KNOWN_DIFF; i++) {
        if ((known_diff[i].loop == loop) && (known_diff[i].col == col)) {
            return true;
        }
    }
    return false;
}

static c64_t c64;
static uint8_t screen[1024];    // screen RAM snapshot taken at the test's verdict write
static uint8_t color[1024];     // ...and color RAM, for the cross-check

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

// 'cia-icr-test2-oneshot.prg' -> '<dir>/icr2-oneshot.ref' (the continues build
// compares against the same reference)
static void make_ref_path(const char* prg_path, char* out, size_t out_size) {
    const char* slash = strrchr(prg_path, '/');
    const char* name = slash ? slash + 1 : prg_path;
    const int dir_len = (int)(name - prg_path);
    snprintf(out, out_size, "%.*sicr2-oneshot.ref", dir_len, prg_path);
}

// boot, quickload the test, and run it until it writes its verdict
static bool run_program(const char* path, int boot_frames, int trace_latch, int trace_loop, uint8_t* out_result) {
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
    /*  --trace support: every measurement starts with 'stx $dc04' (the latch
        low byte = the column), so the write of the traced latch value to
        $DC04 identifies the column, and the three loops write it once each,
        in order.
    */
    int trace_count = 0;
    int trace_left = 0;
    // tick-accurate from here on, the test runs forever and starts over with
    // the next measurement pass right after the verdict write
    for (uint32_t i = 0; i < MAX_RUN_TICKS; i++) {
        c64.pins = _c64_tick(&c64, c64.pins);
        if (trace_loop > 0) {
            if ((0 == (c64.pins & M6502_RW)) && (M6502_GET_ADDR(c64.pins) == 0xDC04) &&
                (M6502_GET_DATA(c64.pins) == (uint8_t)trace_latch))
            {
                if (++trace_count == trace_loop) {
                    trace_left = TRACE_CYCLES;
                    printf("trace of latch $%02X in loop %d (cycle / bus / cpu / cia1):\n",
                        trace_latch, trace_loop);
                }
            }
            if (trace_left > 0) {
                const m6526_t* cia = &c64.cia_1;
                printf("%4d  %04X %c %02X%s  PC=%04X A=%02X  ta=%04X t_out=%d icr=%02X imr=%02X imr1=%02X pip=%06X  IRQ=%d\n",
                    TRACE_CYCLES - trace_left,
                    M6502_GET_ADDR(c64.pins),
                    (c64.pins & M6502_RW) ? 'r' : 'w',
                    M6502_GET_DATA(c64.pins),
                    (c64.pins & M6502_SYNC) ? " SYNC" : "     ",
                    c64.cpu.PC, c64.cpu.A,
                    cia->ta.counter, cia->ta.t_out ? 1 : 0,
                    cia->intr.icr, cia->intr.imr, cia->intr.imr1, cia->intr.pip,
                    (c64.pins & M6502_IRQ) ? 1 : 0);
                trace_left--;
            }
        }
        if ((0 == (c64.pins & M6502_RW)) && (M6502_GET_ADDR(c64.pins) == RESULT_ADDR)) {
            *out_result = M6502_GET_DATA(c64.pins);
            memcpy(screen, &c64.ram[SCREEN_ADDR], sizeof(screen));
            memcpy(color, c64.color_ram, sizeof(color));
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
    int trace_latch = 0;
    int trace_loop = 0;
    bool verbose = false;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (0 == strncmp(arg, "--ref=", 6)) {
            ref_path = arg + 6;
        }
        else if (0 == strncmp(arg, "--boot=", 7)) {
            boot_frames = atoi(arg + 7);
        }
        else if (0 == strncmp(arg, "--trace=", 8)) {
            // --trace=LATCH[:LOOP], latch value in hex like the table columns
            trace_latch = (int)strtol(arg + 8, 0, 16);
            const char* colon = strchr(arg, ':');
            trace_loop = colon ? atoi(colon + 1) : 1;
        }
        else if (0 == strcmp(arg, "--verbose")) {
            verbose = true;    // also dump the loops that pass
        }
        else if (arg[0] == '-') {
            fprintf(stderr, "usage: c64-icr2test [--ref=FILE] [--boot=N] [--trace=HH[:LOOP]] [--verbose] <cia-icr-test2-*.prg>\n");
            return 2;
        }
        else {
            prg_path = arg;
        }
    }
    if (!prg_path) {
        fprintf(stderr, "usage: c64-icr2test [--ref=FILE] [--boot=N] [--trace=HH[:LOOP]] [--verbose] <cia-icr-test2-*.prg>\n");
        return 2;
    }

    char ref_buf[1024];
    if (!ref_path) {
        make_ref_path(prg_path, ref_buf, sizeof(ref_buf));
        ref_path = ref_buf;
    }
    else {
        check_known_diff = false;
    }
    size_t ref_size = 0;
    uint8_t* ref_data = load_file(ref_path, &ref_size);
    if (!ref_data || (ref_size < (REF_SKIP + 17 * NUM_COLS))) {
        fprintf(stderr, "cannot load reference '%s'\n", ref_path);
        return 2;
    }
    const uint8_t* ref = ref_data + REF_SKIP;

    uint8_t result = 0xFF;
    if (!run_program(prg_path, boot_frames, trace_latch, trace_loop, &result)) {
        return 2;
    }

    printf("%s (reference %s)\n\n", prg_path, ref_path);
    int total_diff = 0;
    int total_unexpected = 0;
    for (int l = 0; l < 3; l++) {
        const int base = loops[l].row * NUM_COLS;
        int num_diff = 0;
        int num_unexpected = 0;
        for (int x = 0; x < NUM_COLS; x++) {
            if (screen[base + x] != ref[base + x]) {
                num_diff++;
                if (!is_known_diff(l, x)) {
                    num_unexpected++;
                }
            }
            else if (is_known_diff(l, x)) {
                // a known diff that went away also needs attention
                num_unexpected++;
            }
        }
        total_diff += num_diff;
        total_unexpected += num_unexpected;
        printf("[%s] loop %d: %s", num_unexpected ? "FAIL" : "PASS", l + 1, loops[l].name);
        if (num_diff) {
            printf(" - %d of %d differ", num_diff, NUM_COLS);
        }
        if (num_unexpected) {
            printf(" (%d unexpected)", num_unexpected);
        }
        putchar('\n');
        if (!num_diff && !verbose) {
            continue;
        }
        // one column per timer A latch value, $00 on the left
        printf("    latch:");
        for (int x = 0; x < NUM_COLS; x++) {
            printf(" %02X", x);
        }
        printf("\n    got:  ");
        for (int x = 0; x < NUM_COLS; x++) {
            printf(" %02X", screen[base + x]);
        }
        printf("\n    exp:  ");
        for (int x = 0; x < NUM_COLS; x++) {
            printf(" %02X", ref[base + x]);
        }
        // '~~' marks an expected difference, '^^' one that isn't in known_diff[]
        printf("\n          ");
        for (int x = 0; x < NUM_COLS; x++) {
            const bool diff = (screen[base + x] != ref[base + x]);
            printf(" %s", diff ? (is_known_diff(l, x) ? "~~" : "^^") : "  ");
        }
        printf("\n\n");
    }

    // cross-check against the test program's own verdict, it should agree
    int prg_diff = 0;
    for (int l = 0; l < 3; l++) {
        for (int x = 0; x < NUM_COLS; x++) {
            if ((color[loops[l].row * NUM_COLS + x] & 0x0F) == 10) {
                prg_diff++;
            }
        }
    }
    printf("\n%d of %d compared cells differ (test program itself flagged %d, wrote $%02X to $D7FF)\n",
        total_diff, 3 * NUM_COLS, prg_diff, result);
    free(ref_data);
    if (total_unexpected == 0) {
        if (check_known_diff) {
            printf("=> PASSED (%d known differences, see known_diff[])\n", NUM_KNOWN_DIFF);
        }
        else {
            printf("=> PASSED\n");
        }
        return 0;
    }
    printf("=> FAILED (%d unexpected differences)\n", total_unexpected);
    return 1;
}
