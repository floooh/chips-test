//------------------------------------------------------------------------------
//  c64-wltest.c
//
//  LLM MAINTAINED
//
//  Runs Wolfgang Lorenz C64 test suite programs on a *full* C64 emulation and
//  streams the test's KERNAL text output to stdout, line by line, as it is
//  produced.
//
//  This is the companion to m6502-wltest.c: that one runs the CPU-only tests
//  against a synthetic KERNAL in a bare m6502 + mem setup, which can't run the
//  tests that need the rest of the machine (cia*, icr*, irq, nmi, cnt*,
//  oneshot, mmu, cputiming, trap*, ...). Those are the ones this tool is for.
//
//  The test .prg is quickloaded into a booted C64, started via RUN, and the
//  emulation is watched through the chips debug hook for three KERNAL entry
//  points: CHROUT (character output), LOAD (the test chain-loading its
//  successor, i.e. it finished) and GETIN (the test suite's error path waits
//  for a keypress).
//
//      c64-wltest tests/testsuite-2.15/bin/cia1pb6
//      c64-wltest --screen cia1pb6 cia1pb7
//      c64-wltest --quiet tests/testsuite-2.15/bin/*
//
//  Exit code is 0 if every test passed, 1 otherwise.
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

// emulate in 60Hz frame slices
#define FRAME_USEC (16667)
// frames to let the C64 boot before quickloading the test
#define BOOT_FRAMES (180)
// default give-up limits, these only act as a backstop for a genuinely hung
// emulation: a test that fails the normal way is caught by the GETIN trap.
// 'cia1ta' and friends are the slowest of the bunch, they run ~40 seconds
// without printing a single character.
#define DEF_MAX_FRAMES (12000)      // 200 seconds of emulated time
#define DEF_STALL_FRAMES (3600)     // 60 seconds without any character output
// directory searched when an argument isn't a path to an existing file
#define FALLBACK_DIR "tests/testsuite-2.15/bin"

// KERNAL entry points watched via the debug hook
#define TRAP_LOAD   (0xE16F)    // BASIC LOAD, the test is chain-loading its successor
#define TRAP_CHROUT (0xFFD2)    // print the character in A
#define TRAP_GETIN  (0xFFE4)    // read a key, the test suite does this after an error

// conversion table from C64 screen code to ASCII (the 'x' is the pound sign),
// only used for the optional --screen dump
static const char font_map[65] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[x]   !\"#$%&`()*+,-./0123456789:;<=>?";

typedef struct {
    int max_frames;
    int stall_frames;
    int max_errors;             // >0: keep going past errors, up to this many
    bool dump_screen;
    bool quiet;
} opts_t;

static struct {
    c64_t c64;
    bool stopped;               // must be valid, the chips debug hook dereferences it
    const opts_t* opts;
    int frame;
    int last_output_frame;      // frame of the most recent CHROUT
    bool capture;               // false while the C64 is still booting
    bool running;               // true once the test itself starts printing
    char line[256];             // CHROUT characters are collected here until CR
    int line_pos;
    int num_lines;
    bool saw_ok;                // a line matched '- OK'
    bool saw_failed;            // a line matched 'FAILED' or 'ERROR'
    bool trap_load;             // test finished and wants to load the next one
    bool trap_getin;            // test stopped and waits for a key (error path)
    bool want_key;              // --continue: feed the waiting test a keypress
    int num_errors;             // number of keypresses fed so far
    char next_test[32];         // filename the test tried to chain-load
} state;

// hacky PETSCII to ASCII conversion, returns 0 for characters to drop
static char petscii2ascii(uint8_t p) {
    if ((p >= 0x41) && (p <= 0x5A)) {
        return (char)('A' + (p - 0x41));    // unshifted letters
    }
    else if ((p >= 0xC1) && (p <= 0xDA)) {
        return (char)('A' + (p - 0xC1));    // shifted letters
    }
    else if ((p >= 0x20) && (p < 0x60)) {
        return (char)p;                     // digits and punctuation
    }
    else {
        return 0;                           // control and graphics characters
    }
}

// a complete line of test output has been collected, inspect and print it
static void flush_line(void) {
    state.line[state.line_pos] = 0;
    while ((state.line_pos > 0) && (state.line[state.line_pos-1] == ' ')) {
        state.line[--state.line_pos] = 0;
    }
    if ((state.line_pos > 0) && state.capture) {
        // the success marker is '<testname> - ok', but the amount of whitespace
        // varies (some tests pad the name out), so match on a squeezed copy
        char squeezed[sizeof(state.line)];
        int n = 0;
        for (int i = 0; state.line[i] != 0; i++) {
            if ((state.line[i] != ' ') || ((n > 0) && (squeezed[n-1] != ' '))) {
                squeezed[n++] = state.line[i];
            }
        }
        squeezed[n] = 0;
        if (strstr(squeezed, "- OK")) {
            state.saw_ok = true;
        }
        if (strstr(squeezed, "FAILED") || strstr(squeezed, "ERROR")) {
            state.saw_failed = true;
        }
        state.num_lines++;
        if (!state.opts->quiet) {
            // stream the line out immediately, this is the whole point of the tool
            printf("  | %s\n", state.line);
            fflush(stdout);
        }
    }
    state.line_pos = 0;
}

static void handle_chrout(uint8_t chr) {
    state.running = true;
    state.last_output_frame = state.frame;
    if (chr == 0x0D) {
        flush_line();
    }
    else {
        char c = petscii2ascii(chr);
        if ((c != 0) && (state.line_pos < (int)sizeof(state.line)-1)) {
            state.line[state.line_pos++] = c;
        }
    }
}

// the test called LOAD to chain-load the next test, which means it is done,
// pick up the filename it asked for (from the KERNAL filename pointer/length)
static void handle_load(void) {
    uint8_t l = mem_rd(&state.c64.mem_cpu, 0x00BB);
    uint8_t h = mem_rd(&state.c64.mem_cpu, 0x00BC);
    uint16_t addr = (uint16_t)((h<<8)|l);
    int len = mem_rd(&state.c64.mem_cpu, 0x00B7);
    if (len > (int)sizeof(state.next_test)-1) {
        len = (int)sizeof(state.next_test)-1;
    }
    int pos = 0;
    for (int i = 0; i < len; i++) {
        char c = petscii2ascii(mem_rd(&state.c64.mem_cpu, addr++));
        if (c != 0) {
            state.next_test[pos++] = c;
        }
    }
    state.next_test[pos] = 0;
    state.trap_load = true;
    state.stopped = true;
}

static void debug_cb(void* user_data, uint64_t pins) {
    (void)user_data;
    if (0 == (pins & M6502_SYNC)) {
        return;
    }
    // some tests (trap*, mmu, cpuport) bank the KERNAL out and run code in the
    // RAM underneath it, don't mistake that for a KERNAL call
    if (0 == (state.c64.cpu_port & C64_CPUPORT_HIRAM)) {
        return;
    }
    switch (M6502_GET_ADDR(pins)) {
        case TRAP_CHROUT:
            handle_chrout(state.c64.cpu.A);
            break;
        case TRAP_LOAD:
            handle_load();
            break;
        case TRAP_GETIN:
            // only meaningful once the test is printing, BASIC polls the
            // keyboard too while it's getting the injected RUN out of the way
            if (state.running) {
                if (state.opts->max_errors > 0) {
                    state.want_key = true;
                }
                else {
                    state.trap_getin = true;
                }
                state.stopped = true;
            }
            break;
        default:
            break;
    }
}

// print the 40x25 text screen, for when the streamed output isn't enough
static void dump_screen(void) {
    puts("  +----------------------------------------+");
    for (int y = 0; y < 25; y++) {
        char line[41];
        for (int x = 0; x < 40; x++) {
            uint8_t font_code = mem_rd(&state.c64.mem_vic, (uint16_t)(0x0400 + y*40 + x));
            line[x] = font_map[font_code & 63];
        }
        line[40] = 0;
        printf("  |%s|\n", line);
    }
    puts("  +----------------------------------------+");
}

// load a file into a heap buffer, falls back to FALLBACK_DIR for bare test names
static uint8_t* load_file(const char* path, size_t* out_size, char* out_name, size_t out_name_size) {
    *out_size = 0;
    FILE* fp = fopen(path, "rb");
    if (!fp && !strchr(path, '/')) {
        char fallback[512];
        snprintf(fallback, sizeof(fallback), "%s/%s", FALLBACK_DIR, path);
        fp = fopen(fallback, "rb");
    }
    if (!fp) {
        return 0;
    }
    // the display name is the basename of the path
    const char* slash = strrchr(path, '/');
    snprintf(out_name, out_name_size, "%s", slash ? slash+1 : path);

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t* ptr = 0;
    if (size > 2) {
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

// returns true if the test passed
static bool run_test(const char* path, const opts_t* opts) {
    char name[64];
    size_t size = 0;
    uint8_t* data = load_file(path, &size, name, sizeof(name));
    if (!data) {
        printf("[ERR ] %s - cannot load file\n", path);
        return false;
    }

    memset(&state, 0, sizeof(state));
    state.opts = opts;
    c64_init(&state.c64, &(c64_desc_t){
        .debug = {
            .callback = { .func = debug_cb },
            .stopped = &state.stopped,
        },
        .roms = {
            .chars = { .ptr=dump_c64_char_bin, .size=sizeof(dump_c64_char_bin) },
            .basic = { .ptr=dump_c64_basic_bin, .size=sizeof(dump_c64_basic_bin) },
            .kernal = { .ptr=dump_c64_kernalv3_bin, .size=sizeof(dump_c64_kernalv3_bin) }
        }
    });

    printf("=== %s ===\n", name);
    fflush(stdout);

    // boot the C64, then inject the test and type RUN
    for (int i = 0; i < BOOT_FRAMES; i++) {
        c64_exec(&state.c64, FRAME_USEC);
        state.frame++;
    }
    bool loaded = c64_quickload(&state.c64, (chips_range_t){ .ptr = data, .size = size });
    free(data);
    if (!loaded) {
        printf("[ERR ] %s - c64_quickload() failed\n", name);
        return false;
    }
    c64_basic_run(&state.c64);

    // everything printed from here on belongs to the test
    state.line_pos = 0;
    state.num_lines = 0;
    state.running = false;
    state.capture = true;
    state.last_output_frame = state.frame;

    const char* reason = "no result within frame budget";
    while (state.frame < opts->max_frames) {
        c64_exec(&state.c64, FRAME_USEC);
        state.frame++;
        if (state.trap_load) {
            reason = "finished";
            break;
        }
        if (state.trap_getin) {
            reason = "stopped waiting for a key";
            break;
        }
        if (state.want_key) {
            // --continue: the test hit an error and is waiting for a keypress
            // before it moves on, stuff a SPACE into the KERNAL keyboard buffer
            // so it carries on and reports the remaining errors too
            state.want_key = false;
            state.saw_failed = true;
            state.num_errors++;
            if (state.num_errors >= opts->max_errors) {
                reason = "error limit reached";
                break;
            }
            mem_wr(&state.c64.mem_cpu, 0x0277, 0x20);
            mem_wr(&state.c64.mem_cpu, 0x00C6, 1);
        }
        if ((opts->stall_frames > 0) && ((state.frame - state.last_output_frame) > opts->stall_frames)) {
            reason = "no output, looks hung";
            break;
        }
    }
    // a final line that never got its CR
    flush_line();

    const bool passed = state.saw_ok && !state.saw_failed;
    printf("[%s] %s - %s (%d line%s, %d frames, %.1fs emulated)",
        passed ? "PASS" : "FAIL",
        name,
        reason,
        state.num_lines,
        (state.num_lines == 1) ? "" : "s",
        state.frame,
        (double)state.frame / 60.0);
    if (state.trap_load && (state.next_test[0] != 0)) {
        printf(", next would be '%s'", state.next_test);
    }
    if (state.num_errors > 0) {
        printf(", %d error%s", state.num_errors, (state.num_errors == 1) ? "" : "s");
    }
    putchar('\n');
    if (opts->dump_screen || (!passed && !opts->quiet)) {
        dump_screen();
    }
    fflush(stdout);
    return passed;
}

static void print_help(void) {
    puts("run Wolfgang Lorenz C64 test suite programs on a full C64 emulation\n");
    puts("usage: c64-wltest [options] <file|testname>...\n");
    puts("  a bare test name (no '/') is looked up in " FALLBACK_DIR "\n");
    puts("options:");
    puts("  --frames=N   give up after N 60Hz frames      (default 12000, 200s emulated)");
    puts("  --stall=N    give up after N frames without");
    puts("               any character output, 0 disables (default 3600, 60s emulated)");
    puts("  --screen     always dump the 40x25 text screen");
    puts("               (it is dumped on failure anyway)");
    puts("  --continue[=N]  don't stop at the first error, feed the test a");
    puts("               keypress and let it report up to N errors (default 32)");
    puts("  --quiet      only print the per-test verdict lines");
    puts("  --help\n");
    puts("examples:");
    puts("  c64-wltest cia1pb6");
    puts("  c64-wltest --screen tests/testsuite-2.15/bin/nmi");
    puts("  c64-wltest --quiet tests/testsuite-2.15/bin/cia1*");
}

int main(int argc, char* argv[]) {
    opts_t opts = {
        .max_frames = DEF_MAX_FRAMES,
        .stall_frames = DEF_STALL_FRAMES,
    };
    // at most one test file per argument
    const char** files = malloc((size_t)argc * sizeof(const char*));
    int num_files = 0;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (0 == strncmp(arg, "--frames=", 9)) {
            opts.max_frames = atoi(arg + 9);
        }
        else if (0 == strncmp(arg, "--stall=", 8)) {
            opts.stall_frames = atoi(arg + 8);
        }
        else if (0 == strcmp(arg, "--screen")) {
            opts.dump_screen = true;
        }
        else if (0 == strcmp(arg, "--continue")) {
            opts.max_errors = 32;
        }
        else if (0 == strncmp(arg, "--continue=", 11)) {
            opts.max_errors = atoi(arg + 11);
        }
        else if (0 == strcmp(arg, "--quiet")) {
            opts.quiet = true;
        }
        else if ((0 == strcmp(arg, "--help")) || (0 == strcmp(arg, "-h"))) {
            print_help();
            return 0;
        }
        else if (arg[0] == '-') {
            fprintf(stderr, "unknown option '%s' (try --help)\n", arg);
            return 2;
        }
        else {
            files[num_files++] = arg;
        }
    }
    if (num_files == 0) {
        print_help();
        return 2;
    }

    int num_passed = 0;
    for (int i = 0; i < num_files; i++) {
        if (run_test(files[i], &opts)) {
            num_passed++;
        }
    }
    printf("\n%d/%d passed, %d failed\n", num_passed, num_files, num_files - num_passed);
    free(files);
    return (num_passed == num_files) ? 0 : 1;
}
