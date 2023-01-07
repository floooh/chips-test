//------------------------------------------------------------------------------
//  c64-bench.c
//  Unthrottled headless C64 emu for benchmarking / profiling.
//------------------------------------------------------------------------------
#include <stdio.h>
#define SOKOL_IMPL
#include "sokol_time.h"
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

static struct {
    c64_t c64;
} state;

#define NUM_USEC (5*1000000)

static void dummy_audio_callback(const float* samples, int num_samples, void* user_data) {
    (void)samples;
    (void)num_samples;
    (void)user_data;
};

int main() {
    /* provide "throw-away" pixel buffer and audio callback, so
       that the video and audio generation isn't skipped in the
       emulator
    */
    c64_init(&state.c64, &(c64_desc_t){
        .audio.callback.func = dummy_audio_callback,
        .roms = {
            .chars = { .ptr=dump_c64_char_bin, .size=sizeof(dump_c64_char_bin) },
            .basic = { .ptr=dump_c64_basic_bin, .size=sizeof(dump_c64_basic_bin) },
            .kernal = { .ptr=dump_c64_kernalv3_bin, .size=sizeof(dump_c64_kernalv3_bin) }
        }
    });
    stm_setup();
    printf("== running emulation for %.2f emulated secs\n", NUM_USEC / 1000000.0);
    uint64_t start = stm_now();
    c64_exec(&state.c64, NUM_USEC);
    printf("== time: %f sec\n", stm_sec(stm_since(start)));
    return 0;
}
