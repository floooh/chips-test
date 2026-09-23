//------------------------------------------------------------------------------
//  c64-vicetest.c
//
//  LLM MAINTAINED
//
//  Runs VIC-II test programs from the VICE testprogs collection
//  (tests/vice-tests/VICII) on the chips C64 emulation, captures the VIC-II
//  framebuffer and compares it against the reference screenshot that ships
//  with the test.
//
//  This is the visual counterpart to c64-wltest.c: the Wolfgang Lorenz tests
//  report pass/fail as text through CHROUT, but almost all VIC-II tests are
//  pictures - the raster effect either lands on the right cycle or it doesn't,
//  and you can only tell by looking at the output.
//
//      c64-vicetest tests/vice-tests/VICII/D011Test/disable-bad.prg
//      c64-vicetest --dump-dir=/tmp/shots tests/vice-tests/VICII/border/*.prg
//      c64-vicetest --crc tests/vice-tests/VICII/*/*.prg > baseline.txt
//
//  Reference lookup follows the VICE convention: the reference for
//  '<dir>/<name>.prg' is '<dir>/references/<name>.prg.png'. Most tests don't
//  have one. For those the tool only reports a CRC of the captured frame,
//  which is still useful: '--crc' output piped through diff tells you exactly
//  which tests a VIC-II change altered, even when there is nothing to say
//  whether the new output is *right*.
//
//  Exit code is 0 if no test with a reference image failed.
//
//  NOTE: reference images are compared in palette-index space, not RGB. VICE
//  and chips use different RGB values for the 16 C64 colors, so each distinct
//  color in the reference is first mapped to its nearest chips palette entry.
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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// emulate in ~60Hz slices while booting, the capture itself is raster-synced
#define SLICE_USEC (20000)
// frames to let the C64 boot before quickloading the test
#define DEF_BOOT_FRAMES (180)
// frames to let the test run before the frame is captured
#define DEF_RUN_FRAMES (60)
// give up raster-syncing after this many ticks (more than two PAL frames)
#define MAX_SYNC_TICKS (64 * 1024)

// Alignment between the chips framebuffer and a VICE reference screenshot.
// chips renders a 392x272 window starting at pixel 64 of the 504 pixel PAL
// raster, VICE crops 384x272 at a slightly different origin. Calibrated with
// --find-offset against D011Test/disable-bad.prg, which matches its reference
// exactly at this offset (0.00% differing pixels, >3.4% at every neighbour).
#define DEF_REF_OFFSET_X (0)
#define DEF_REF_OFFSET_Y (1)

// VICE reference screenshots for PAL are this size, anything else (NTSC) is skipped
#define PAL_REF_HEIGHT (272)

typedef struct {
    int boot_frames;
    int run_frames;
    int off_x, off_y;
    float tolerance;            // percent of differing pixels still considered a pass
    const char* ref_path;       // explicit reference, overrides the auto lookup
    const char* dump_dir;
    const char* diff_dir;
    bool no_ref;
    bool find_offset;
    bool crc_only;
    bool quiet;
} opts_t;

static struct {
    c64_t c64;
    int width, height;          // dimensions of the captured frame
    uint8_t* pixels;            // captured frame, one palette index per pixel
    const uint32_t* palette;    // chips C64 palette, RGBA8
    int palette_size;
} state;

//--- small helpers ------------------------------------------------------------

static uint32_t crc32(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (uint32_t)(-(int32_t)(crc & 1)));
        }
    }
    return ~crc;
}

// load a file into a heap buffer
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

static const char* basename_of(const char* path) {
    const char* slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

// '<dir>/<name>.prg' -> '<dir>/references/<name>.prg.png', the VICE convention
static void make_ref_path(const char* prg_path, char* out, size_t out_size) {
    const char* name = basename_of(prg_path);
    int dir_len = (int)(name - prg_path);
    snprintf(out, out_size, "%.*sreferences/%s.png", dir_len, prg_path, name);
}

static void make_out_path(const char* dir, const char* prg_path, const char* suffix, char* out, size_t out_size) {
    snprintf(out, out_size, "%s/%s%s.png", dir, basename_of(prg_path), suffix);
}

//--- emulation ----------------------------------------------------------------

// run until the raster unit wraps around to the top of the frame, so that a
// capture never catches a half-rendered frame
static bool run_to_frame_start(void) {
    uint16_t prev = state.c64.vic.rs.v_count;
    for (int i = 0; i < MAX_SYNC_TICKS; i++) {
        c64_exec(&state.c64, 1);
        const uint16_t cur = state.c64.vic.rs.v_count;
        if (cur < prev) {
            return true;
        }
        prev = cur;
    }
    return false;
}

// copy the visible part of the VIC-II framebuffer into state.pixels
static void capture_frame(void) {
    const chips_display_info_t info = c64_display_info(&state.c64);
    state.width = (int)info.screen.width;
    state.height = (int)info.screen.height;
    state.palette = (const uint32_t*)info.palette.ptr;
    state.palette_size = (int)(info.palette.size / sizeof(uint32_t));
    state.pixels = realloc(state.pixels, (size_t)(state.width * state.height));
    const uint8_t* src = (const uint8_t*)info.frame.buffer.ptr;
    const int stride = (int)info.frame.dim.width;
    for (int y = 0; y < state.height; y++) {
        memcpy(state.pixels + y * state.width,
               src + ((int)info.screen.y + y) * stride + (int)info.screen.x,
               (size_t)state.width);
    }
}

// boot a fresh C64, quickload the test and let it run
static bool run_program(const char* path, const opts_t* opts) {
    size_t size = 0;
    uint8_t* data = load_file(path, &size);
    if (!data) {
        return false;
    }
    memset(&state.c64, 0, sizeof(state.c64));
    c64_init(&state.c64, &(c64_desc_t){
        .roms = {
            .chars = { .ptr=dump_c64_char_bin, .size=sizeof(dump_c64_char_bin) },
            .basic = { .ptr=dump_c64_basic_bin, .size=sizeof(dump_c64_basic_bin) },
            .kernal = { .ptr=dump_c64_kernalv3_bin, .size=sizeof(dump_c64_kernalv3_bin) }
        }
    });
    for (int i = 0; i < opts->boot_frames; i++) {
        c64_exec(&state.c64, SLICE_USEC);
    }
    const bool loaded = c64_quickload(&state.c64, (chips_range_t){ .ptr = data, .size = size });
    free(data);
    if (!loaded) {
        return false;
    }
    c64_basic_run(&state.c64);
    for (int i = 0; i < opts->run_frames; i++) {
        c64_exec(&state.c64, SLICE_USEC);
    }
    run_to_frame_start();
    run_to_frame_start();   // one full frame rendered from a known start
    capture_frame();
    return true;
}

//--- image output -------------------------------------------------------------

static bool write_png(const char* path, const uint8_t* indices, int w, int h) {
    uint32_t* rgba = malloc((size_t)(w * h) * sizeof(uint32_t));
    for (int i = 0; i < w * h; i++) {
        const int idx = indices[i];
        rgba[i] = (idx < state.palette_size) ? state.palette[idx] : 0xFF000000;
    }
    const int ok = stbi_write_png(path, w, h, 4, rgba, w * 4);
    free(rgba);
    return ok != 0;
}

//--- reference comparison -----------------------------------------------------

typedef struct {
    uint8_t* indices;           // one chips palette index per pixel
    int width, height;
    int num_colors;             // distinct colors found in the reference
} ref_image_t;

static uint8_t nearest_palette_index(uint8_t r, uint8_t g, uint8_t b) {
    int best = 0, best_dist = 1 << 30;
    for (int i = 0; i < state.palette_size; i++) {
        const uint32_t c = state.palette[i];
        const int dr = (int)(c & 0xFF) - r;
        const int dg = (int)((c >> 8) & 0xFF) - g;
        const int db = (int)((c >> 16) & 0xFF) - b;
        const int dist = dr*dr + dg*dg + db*db;
        if (dist < best_dist) {
            best_dist = dist;
            best = i;
        }
    }
    return (uint8_t)best;
}

// load a VICE reference screenshot and convert it to chips palette indices
static bool load_reference(const char* path, ref_image_t* out) {
    memset(out, 0, sizeof(*out));
    int w = 0, h = 0, comps = 0;
    uint8_t* rgba = stbi_load(path, &w, &h, &comps, 4);
    if (!rgba) {
        return false;
    }
    out->width = w;
    out->height = h;
    out->indices = malloc((size_t)(w * h));
    // a C64 screenshot has at most 16 distinct colors, cache the mapping
    uint32_t cache_key[64];
    uint8_t cache_val[64];
    int num_cached = 0;
    for (int i = 0; i < w * h; i++) {
        const uint32_t key = ((uint32_t)rgba[i*4+0]) | ((uint32_t)rgba[i*4+1] << 8) | ((uint32_t)rgba[i*4+2] << 16);
        int slot = -1;
        for (int c = 0; c < num_cached; c++) {
            if (cache_key[c] == key) {
                slot = c;
                break;
            }
        }
        if (slot < 0) {
            const uint8_t idx = nearest_palette_index(rgba[i*4+0], rgba[i*4+1], rgba[i*4+2]);
            if (num_cached < 64) {
                cache_key[num_cached] = key;
                cache_val[num_cached] = idx;
                slot = num_cached++;
            }
            else {
                out->indices[i] = idx;
                continue;
            }
        }
        out->indices[i] = cache_val[slot];
    }
    out->num_colors = num_cached;
    stbi_image_free(rgba);
    return true;
}

typedef struct {
    int num_diff;
    int num_compared;
} compare_result_t;

// reference pixel (rx,ry) is compared against captured pixel (rx+dx, ry+dy)
static compare_result_t compare_to_ref(const ref_image_t* ref, int dx, int dy) {
    compare_result_t res = {0};
    for (int ry = 0; ry < ref->height; ry++) {
        const int cy = ry + dy;
        if ((cy < 0) || (cy >= state.height)) {
            continue;
        }
        for (int rx = 0; rx < ref->width; rx++) {
            const int cx = rx + dx;
            if ((cx < 0) || (cx >= state.width)) {
                continue;
            }
            res.num_compared++;
            if (ref->indices[ry * ref->width + rx] != state.pixels[cy * state.width + cx]) {
                res.num_diff++;
            }
        }
    }
    return res;
}

// brute-force the alignment that minimizes the pixel difference, used to
// calibrate DEF_REF_OFFSET_X/Y once against a known-good test
static void find_best_offset(const ref_image_t* ref, int* out_dx, int* out_dy, float* out_percent) {
    float best = 101.0f;
    *out_dx = 0;
    *out_dy = 0;
    for (int dy = -16; dy <= 16; dy++) {
        for (int dx = -24; dx <= 24; dx++) {
            const compare_result_t r = compare_to_ref(ref, dx, dy);
            if (r.num_compared < (ref->width * ref->height) / 2) {
                continue;   // too little overlap to be meaningful
            }
            const float percent = (100.0f * (float)r.num_diff) / (float)r.num_compared;
            if (percent < best) {
                best = percent;
                *out_dx = dx;
                *out_dy = dy;
            }
        }
    }
    *out_percent = best;
}

// red where the reference and the capture disagree, dimmed capture elsewhere
static bool write_diff_png(const char* path, const ref_image_t* ref, int dx, int dy) {
    const int w = state.width, h = state.height;
    uint32_t* rgba = malloc((size_t)(w * h) * sizeof(uint32_t));
    for (int i = 0; i < w * h; i++) {
        const int idx = state.pixels[i];
        const uint32_t c = (idx < state.palette_size) ? state.palette[idx] : 0xFF000000;
        // dim the background so the red difference mask stands out
        rgba[i] = 0xFF000000 | (((c & 0x00FEFEFE) >> 1) & 0x00FEFEFE);
    }
    for (int ry = 0; ry < ref->height; ry++) {
        const int cy = ry + dy;
        if ((cy < 0) || (cy >= h)) {
            continue;
        }
        for (int rx = 0; rx < ref->width; rx++) {
            const int cx = rx + dx;
            if ((cx < 0) || (cx >= w)) {
                continue;
            }
            if (ref->indices[ry * ref->width + rx] != state.pixels[cy * w + cx]) {
                rgba[cy * w + cx] = 0xFF0000FF;
            }
        }
    }
    const int ok = stbi_write_png(path, w, h, 4, rgba, w * 4);
    free(rgba);
    return ok != 0;
}

//--- per-test driver ----------------------------------------------------------

typedef enum { RESULT_PASS, RESULT_FAIL, RESULT_NOREF, RESULT_SKIP, RESULT_ERROR } result_t;

static result_t run_test(const char* path, const opts_t* opts) {
    const char* name = basename_of(path);
    if (!run_program(path, opts)) {
        printf("[ERR ] %s - cannot load or start\n", name);
        return RESULT_ERROR;
    }
    const uint32_t crc = crc32(state.pixels, (size_t)(state.width * state.height));

    if (opts->crc_only) {
        printf("%-40s %08X\n", name, crc);
        return RESULT_NOREF;
    }

    if (opts->dump_dir) {
        char out_path[1024];
        make_out_path(opts->dump_dir, path, "", out_path, sizeof(out_path));
        if (!write_png(out_path, state.pixels, state.width, state.height)) {
            printf("[WARN] %s - cannot write '%s'\n", name, out_path);
        }
    }

    // a single-color frame usually means the program never started
    bool uniform = true;
    for (int i = 1; i < state.width * state.height; i++) {
        if (state.pixels[i] != state.pixels[0]) {
            uniform = false;
            break;
        }
    }

    char ref_path[1024];
    if (opts->ref_path) {
        snprintf(ref_path, sizeof(ref_path), "%s", opts->ref_path);
    }
    else {
        make_ref_path(path, ref_path, sizeof(ref_path));
    }

    ref_image_t ref;
    if (opts->no_ref || !load_reference(ref_path, &ref)) {
        printf("[ -- ] %s - crc %08X, no reference%s\n", name, crc, uniform ? ", BLANK SCREEN" : "");
        return RESULT_NOREF;
    }

    result_t result = RESULT_FAIL;
    if (ref.height != PAL_REF_HEIGHT) {
        printf("[SKIP] %s - reference is %dx%d, not PAL (chips is PAL only)\n", name, ref.width, ref.height);
        result = RESULT_SKIP;
    }
    else if (ref.num_colors > 16) {
        printf("[SKIP] %s - reference has %d distinct colors, not a raw C64 screenshot\n", name, ref.num_colors);
        result = RESULT_SKIP;
    }
    else {
        int dx = opts->off_x, dy = opts->off_y;
        float percent;
        if (opts->find_offset) {
            find_best_offset(&ref, &dx, &dy, &percent);
        }
        else {
            const compare_result_t r = compare_to_ref(&ref, dx, dy);
            percent = r.num_compared ? (100.0f * (float)r.num_diff) / (float)r.num_compared : 100.0f;
        }
        const bool passed = (percent <= opts->tolerance);
        result = passed ? RESULT_PASS : RESULT_FAIL;
        printf("[%s] %s - crc %08X, %.2f%% pixels differ", passed ? "PASS" : "FAIL", name, crc, percent);
        if (opts->find_offset) {
            printf(" at best offset %d,%d", dx, dy);
        }
        if (uniform) {
            printf(", BLANK SCREEN");
        }
        putchar('\n');

        if (opts->diff_dir && !passed) {
            char out_path[1024];
            make_out_path(opts->diff_dir, path, "-diff", out_path, sizeof(out_path));
            if (!write_diff_png(out_path, &ref, dx, dy)) {
                printf("[WARN] %s - cannot write '%s'\n", name, out_path);
            }
        }
    }
    free(ref.indices);
    return result;
}

//--- main ---------------------------------------------------------------------

static void print_help(void) {
    puts("run VICE VIC-II test programs on the chips C64 emulation\n");
    puts("usage: c64-vicetest [options] <file.prg>...\n");
    puts("  the reference screenshot for '<dir>/<name>.prg' is looked up as");
    puts("  '<dir>/references/<name>.prg.png', which is what the VICE testprogs use\n");
    puts("options:");
    puts("  --boot=N          frames to boot the C64 before loading (default 180)");
    puts("  --frames=N        frames to run the test before capturing (default 60)");
    puts("  --ref=FILE        use this reference instead of the automatic lookup");
    puts("  --no-ref          skip reference comparison, only report CRCs");
    puts("  --offset=X,Y      reference alignment in pixels (default 0,1)");
    puts("  --find-offset     brute-force the best alignment and report it,");
    puts("                    use this to calibrate --offset, not to grade a test");
    puts("  --tolerance=P     percent of differing pixels still counted as a pass");
    puts("  --dump-dir=DIR    write every captured frame to DIR as a PNG");
    puts("  --diff-dir=DIR    write a difference image for every failing test");
    puts("  --crc             print only 'name crc' lines, for regression baselines");
    puts("  --quiet           suppress the summary");
    puts("  --help\n");
    puts("examples:");
    puts("  c64-vicetest tests/vice-tests/VICII/D011Test/disable-bad.prg");
    puts("  c64-vicetest --dump-dir=/tmp/shots tests/vice-tests/VICII/border/*.prg");
    puts("  c64-vicetest --crc tests/vice-tests/VICII/*/*.prg > baseline.txt");
}

int main(int argc, char* argv[]) {
    opts_t opts = {
        .boot_frames = DEF_BOOT_FRAMES,
        .run_frames = DEF_RUN_FRAMES,
        .off_x = DEF_REF_OFFSET_X,
        .off_y = DEF_REF_OFFSET_Y,
    };
    const char** files = malloc((size_t)argc * sizeof(const char*));
    int num_files = 0;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (0 == strncmp(arg, "--boot=", 7)) {
            opts.boot_frames = atoi(arg + 7);
        }
        else if (0 == strncmp(arg, "--frames=", 9)) {
            opts.run_frames = atoi(arg + 9);
        }
        else if (0 == strncmp(arg, "--ref=", 6)) {
            opts.ref_path = arg + 6;
        }
        else if (0 == strcmp(arg, "--no-ref")) {
            opts.no_ref = true;
        }
        else if (0 == strncmp(arg, "--offset=", 9)) {
            if (2 != sscanf(arg + 9, "%d,%d", &opts.off_x, &opts.off_y)) {
                fprintf(stderr, "expected --offset=X,Y\n");
                return 2;
            }
        }
        else if (0 == strcmp(arg, "--find-offset")) {
            opts.find_offset = true;
        }
        else if (0 == strncmp(arg, "--tolerance=", 12)) {
            opts.tolerance = (float)atof(arg + 12);
        }
        else if (0 == strncmp(arg, "--dump-dir=", 11)) {
            opts.dump_dir = arg + 11;
        }
        else if (0 == strncmp(arg, "--diff-dir=", 11)) {
            opts.diff_dir = arg + 11;
        }
        else if (0 == strcmp(arg, "--crc")) {
            opts.crc_only = true;
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
    if (opts.ref_path && (num_files > 1)) {
        fprintf(stderr, "--ref only makes sense with a single test\n");
        return 2;
    }

    int num[5] = {0};
    for (int i = 0; i < num_files; i++) {
        num[run_test(files[i], &opts)]++;
        fflush(stdout);
    }
    if (!opts.quiet && !opts.crc_only) {
        printf("\n%d passed, %d failed, %d without reference, %d skipped, %d errors\n",
            num[RESULT_PASS], num[RESULT_FAIL], num[RESULT_NOREF], num[RESULT_SKIP], num[RESULT_ERROR]);
    }
    free(files);
    free(state.pixels);
    return ((num[RESULT_FAIL] == 0) && (num[RESULT_ERROR] == 0)) ? 0 : 1;
}
