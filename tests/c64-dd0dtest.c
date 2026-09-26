//------------------------------------------------------------------------------
//  c64-dd0dtest.c
//
//  Runs Wilfred Bos' 'dd0dtest' (tests/vice-tests/CIA/dd0dtest) on the chips
//  C64 emulation and prints the test program's own report.
//
//      c64-dd0dtest tests/vice-tests/CIA/dd0dtest/dd0dtest.prg
//
//  The test checks cycle-exact reading of $DD0D (CIA2 interrupt control) while
//  an NMI is pending, with all the addressing modes that touch the register
//  (abs, abs,x with and without page-cross, read-modify-write). Each of the 19
//  sub-tests prints a line like
//
//      TEST 0C READ  010101010101 FAILED
//           EXPECTED 0101FFFF8181
//
//  The test scrolls its output and finally signals its verdict by writing $00
//  (success) or $FF (failure) to $D7FF, so instead of scraping screen RAM this
//  tool traps every CHROUT ($FFD2) call and reassembles the printed text.
//
//  A handful of sub-tests are known to fail (see KNOWN_FAILED below), so the
//  exit code isn't the test program's own verdict but a comparison against
//  that list: zero as long as exactly the known-bad sub-tests fail. A new
//  failure *and* a known failure that starts passing both break the build (the
//  latter means the list needs updating).
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

#define SLICE_USEC (20000)          // emulate in ~50Hz slices
#define DEF_BOOT_FRAMES (180)       // frames to let the C64 boot before quickloading
#define MAX_RUN_TICKS (60*1000*1000)
#define CHROUT (0xFFD2)
#define RESULT_ADDR (0xD7FF)        // test writes $00 (ok) or $FF (failed) here
#define RESULT_EXTRA_TICKS (500000) // ...keep ticking after the first write to catch the $FF
#define TRACE_CYCLES (170)          // --trace: cycles to dump after a sub-test starts its timer
#define NUM_TESTS (0x19)            // sub-tests are numbered 01..19 (hex)

/*  Sub-tests which are expected to fail - none at the moment.

    The test detects whether it's running on an old or a new CIA (it prints
    which) and picks its expected-value table accordingly, and chips emulates
    the old CIA, so this is a real all-green.
*/
static const uint8_t known_failed[] = { 0 };
#define NUM_KNOWN_FAILED (0)

static c64_t c64;
static char text[64*1024];
static size_t text_len;

/*  --analyze support: what every sub-test does to $DD0D, relative to the timer
    A underflow cycle. Each sub-test is one window starting at its 'force load +
    start' write to $DD0E, and the interesting part is just the handful of ICR
    accesses around the underflow plus whether an NMI was taken.
*/
#define ANALYZE_WINDOW (120)        // cycles to watch after a sub-test starts its timer
#define ANALYZE_MAX_EVENTS (12)
#define ANALYZE_MAX_UNDERFLOWS (8)

typedef struct {
    int cycle;                      // cycle within the window
    bool write;
    uint8_t data;
} icr_access_t;

typedef struct {
    bool active;
    int cycle;                      // cycles since the window started
    int num_u;                      // timer A underflows (it keeps running, so there are several)
    int u[ANALYZE_MAX_UNDERFLOWS];
    bool prev_t_out;
    int nmi;                        // window cycle of the NMI vector fetch, -1 if none
    int num_acc;
    icr_access_t acc[ANALYZE_MAX_EVENTS];
} analyze_t;

static analyze_t analyze[NUM_TESTS + 1];

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

// PETSCII as handed to CHROUT -> ASCII, enough for the test's output
static void put_char(uint8_t c) {
    char out = 0;
    if (c == 0x0D) {
        out = '\n';
    }
    else if ((c >= 0x20) && (c < 0x40)) {
        out = (char)c;
    }
    else if ((c >= 0x41) && (c <= 0x5A)) {
        out = (char)c;          // unshifted PETSCII letters
    }
    else if ((c >= 0xC1) && (c <= 0xDA)) {
        out = (char)(c - 0x80);
    }
    if (out && (text_len < sizeof(text) - 1)) {
        text[text_len++] = out;
    }
}

static bool is_known_failed(uint8_t test) {
    for (int i = 0; i < NUM_KNOWN_FAILED; i++) {
        if (known_failed[i] == test) {
            return true;
        }
    }
    return false;
}

/*  Scan the captured output for the sub-test report lines

        TEST 0B READ  010101010101 OK
        TEST 0C READ  010101010101 FAILED

    (sub-tests 01 and 02 print 'ACK' instead of 'READ') and fill in a
    0=passed / 1=failed state per sub-test, or -1 if the line was missing.
*/
static void scan_results(int8_t* out_state) {
    for (int i = 0; i <= NUM_TESTS; i++) {
        out_state[i] = -1;
    }
    const char* line = text;
    while (line && *line) {
        const char* end = strchr(line, '\n');
        const size_t len = end ? (size_t)(end - line) : strlen(line);
        unsigned int test = 0;
        if ((len > 8) && (0 == strncmp(line, "TEST ", 5)) && (1 == sscanf(line + 5, "%2x", &test)) &&
            (test >= 1) && (test <= NUM_TESTS))
        {
            // don't let a strstr() run past the end of the line
            const bool failed = (0 != strncmp(line + len - 2, "OK", 2));
            out_state[test] = failed ? 1 : 0;
        }
        line = end ? (end + 1) : 0;
    }
}

int main(int argc, char* argv[]) {
    const char* prg_path = 0;
    int boot_frames = DEF_BOOT_FRAMES;
    int trace_test = 0;
    bool do_analyze = false;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (0 == strncmp(arg, "--boot=", 7)) {
            boot_frames = atoi(arg + 7);
        }
        else if (0 == strncmp(arg, "--trace=", 8)) {
            trace_test = (int)strtol(arg + 8, 0, 16);
        }
        else if (0 == strcmp(arg, "--analyze")) {
            do_analyze = true;
        }
        else if (arg[0] == '-') {
            fprintf(stderr, "usage: c64-dd0dtest [--boot=N] [--trace=HH] [--analyze] <dd0dtest.prg>\n");
            return 2;
        }
        else {
            prg_path = arg;
        }
    }
    if (!prg_path) {
        fprintf(stderr, "usage: c64-dd0dtest [--boot=N] [--trace=HH] [--analyze] <dd0dtest.prg>\n");
        return 2;
    }

    size_t size = 0;
    uint8_t* data = load_file(prg_path, &size);
    if (!data) {
        fprintf(stderr, "cannot load '%s'\n", prg_path);
        return 2;
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
        fprintf(stderr, "cannot quickload '%s'\n", prg_path);
        return 2;
    }
    c64_basic_run(&c64);

    /*  Tick-accurate from here on so that every CHROUT call can be trapped.

        Two subtleties with using the SYNC pin for this:
        - SYNC is asserted at the *end* of the previous instruction and stays
          asserted until the opcode fetch is actually executed, so a VIC
          badline (which stalls the CPU via RDY for up to ~45 cycles) would
          otherwise count the same CHROUT call dozens of times - only the
          rising edge counts.
        - an opcode fetch that is hijacked by an interrupt also shows up as
          SYNC at the target address, but the instruction isn't executed (the
          CPU restarts at the same PC after the RTI), so the fetch is only a
          real CHROUT call if brk_flags is clear when SYNC is consumed.
    */
    bool done = false;
    bool prev_sync = (c64.pins & M6502_SYNC) != 0;
    bool pending = false;
    bool have_result = false;
    uint8_t result = 0xFF;
    uint32_t num_aborted = 0;
    uint32_t extra_ticks = 0;
    /*  --trace=HH support: sub-tests 03..16 all start by writing $11 (force
        load + start) to $DD0E, so the Nth such write starts sub-test N+2.
    */
    int trace_trigger = (trace_test >= 3) ? (trace_test - 2) : 0;
    int trace_count = 0;
    int trace_left = 0;
    int sub_test = 0;               // --analyze: sub-test whose window is currently open
    for (uint32_t i = 0; (i < MAX_RUN_TICKS) && !done; i++) {
        c64.pins = _c64_tick(&c64, c64.pins);
        const bool dd0e_start = (0 == (c64.pins & M6502_RW)) &&
            (M6502_GET_ADDR(c64.pins) == 0xDD0E) && (M6502_GET_DATA(c64.pins) == 0x11);
        if (do_analyze) {
            // the Nth 'force load + start' write opens the window of sub-test N+2
            if (dd0e_start) {
                sub_test++;
                const int idx = sub_test + 2;
                if (idx <= NUM_TESTS) {
                    analyze_t* a = &analyze[idx];
                    a->active = true;
                    a->cycle = 0;
                    a->num_u = 0;
                    a->prev_t_out = false;
                    a->nmi = -1;
                    a->num_acc = 0;
                }
            }
            for (int t = 0; t <= NUM_TESTS; t++) {
                analyze_t* a = &analyze[t];
                if (!a->active) {
                    continue;
                }
                if (c64.cia_2.ta.t_out && !a->prev_t_out && (a->num_u < ANALYZE_MAX_UNDERFLOWS)) {
                    a->u[a->num_u++] = a->cycle;
                }
                a->prev_t_out = c64.cia_2.ta.t_out;
                if ((c64.pins & M6502_RW) && (M6502_GET_ADDR(c64.pins) == 0xFFFA) && (a->nmi < 0)) {
                    a->nmi = a->cycle;
                }
                if ((M6502_GET_ADDR(c64.pins) == 0xDD0D) && (a->num_acc < ANALYZE_MAX_EVENTS)) {
                    a->acc[a->num_acc++] = (icr_access_t){
                        .cycle = a->cycle,
                        .write = (0 == (c64.pins & M6502_RW)),
                        .data = M6502_GET_DATA(c64.pins),
                    };
                }
                if (++a->cycle >= ANALYZE_WINDOW) {
                    a->active = false;
                }
            }
        }
        if (trace_trigger) {
            if (dd0e_start) {
                if (++trace_count == trace_trigger) {
                    trace_left = TRACE_CYCLES;
                    printf("trace of sub-test %02X (cycle / bus / cpu / cia2):\n", trace_test);
                }
            }
            if (trace_left > 0) {
                const m6526_t* cia = &c64.cia_2;
                printf("%4d  %04X %c %02X%s%s  PC=%04X A=%02X  ta=%04X t_out=%d icr=%02X imr=%02X pip=%06X  NMI=%d\n",
                    TRACE_CYCLES - trace_left,
                    M6502_GET_ADDR(c64.pins),
                    (c64.pins & M6502_RW) ? 'r' : 'w',
                    M6502_GET_DATA(c64.pins),
                    (c64.pins & M6502_SYNC) ? " SYNC" : "     ",
                    (c64.cpu.brk_flags) ? " BRK" : "    ",
                    c64.cpu.PC, c64.cpu.A,
                    cia->ta.counter, cia->ta.t_out ? 1 : 0,
                    cia->intr.icr, cia->intr.imr, cia->intr.pip,
                    (c64.pins & M6502_NMI) ? 1 : 0);
                trace_left--;
            }
        }
        const bool sync = (c64.pins & M6502_SYNC) != 0;
        if (pending && !sync) {
            // SYNC was consumed in this tick
            if (c64.cpu.brk_flags != 0) {
                num_aborted++;
            }
            else {
                put_char(c64.cpu.A);
            }
            pending = false;
        }
        if (!prev_sync && sync && (M6502_GET_ADDR(c64.pins) == CHROUT)) {
            pending = true;
        }
        prev_sync = sync;
        /*  The test signals its verdict with a write to $D7FF (which is a SID
            mirror, so it never lands in RAM - watch the bus instead). It
            *always* writes $00 when the last sub-test is done and only then
            overwrites it with $FF if any sub-test failed, so keep going for a
            bit and take the last value.
        */
        if ((0 == (c64.pins & M6502_RW)) && (M6502_GET_ADDR(c64.pins) == RESULT_ADDR)) {
            result = M6502_GET_DATA(c64.pins);
            have_result = true;
        }
        if (have_result && (++extra_ticks > RESULT_EXTRA_TICKS)) {
            done = true;
        }
    }
    text[text_len] = 0;
    if (!done) {
        fprintf(stderr, "test did not finish within %u ticks\n", MAX_RUN_TICKS);
        fputs(text, stdout);
        return 2;
    }
    fputs(text, stdout);

    /*  The exit code is a comparison against the known-failed list, not the
        test program's own verdict (which is just 'did everything pass').
    */
    int8_t state[NUM_TESTS + 1];
    scan_results(state);

    /*  --analyze: one line per sub-test with every $DD0D access placed relative
        to the timer A underflow cycle (U), plus whether an NMI was taken. This
        is what pins down the interrupt model: sub-tests with the same
        alignment must behave the same way.
    */
    if (do_analyze) {
        printf("\n$DD0D accesses relative to the nearest preceding timer A underflow (Un+k):\n\n");
        for (int t = 3; t <= NUM_TESTS; t++) {
            const analyze_t* a = &analyze[t];
            printf("  %02X %-6s nmi=%-3s ", t,
                (state[t] < 0) ? "?" : (state[t] ? "FAILED" : "ok"),
                (a->nmi < 0) ? "no" : "yes");
            for (int i = 0; i < a->num_acc; i++) {
                int u = -1;
                for (int k = 0; k < a->num_u; k++) {
                    if (a->u[k] <= a->acc[i].cycle) {
                        u = k;
                    }
                }
                if (u < 0) {
                    printf(" pre%+d:%c%02X", a->acc[i].cycle - ((a->num_u > 0) ? a->u[0] : 0),
                        a->acc[i].write ? 'w' : 'r', a->acc[i].data);
                }
                else {
                    printf(" U%d+%d:%c%02X", u, a->acc[i].cycle - a->u[u],
                        a->acc[i].write ? 'w' : 'r', a->acc[i].data);
                }
            }
            putchar('\n');
        }
        putchar('\n');
    }

    int num_unexpected = 0;
    for (int i = 1; i <= NUM_TESTS; i++) {
        const bool known = is_known_failed((uint8_t)i);
        if (state[i] < 0) {
            printf("\n!! sub-test %02X did not report at all", i);
            num_unexpected++;
        }
        else if ((state[i] != 0) && !known) {
            printf("\n!! sub-test %02X regressed (new failure)", i);
            num_unexpected++;
        }
        else if ((state[i] == 0) && known) {
            printf("\n!! sub-test %02X now passes - remove it from known_failed[]", i);
            num_unexpected++;
        }
    }
    if (num_unexpected == 0) {
        printf("\n=> PASSED (%d known failures", NUM_KNOWN_FAILED);
        for (int i = 0; i < NUM_KNOWN_FAILED; i++) {
            printf("%s%02X", (i > 0) ? "," : ": ", known_failed[i]);
        }
        printf(", %u interrupted CHROUT fetches skipped)\n", num_aborted);
        return 0;
    }
    printf("\n=> FAILED (%d unexpected results, test program verdict: %s)\n",
        num_unexpected, (result == 0) ? "all passed" : "failures");
    return 1;
}
