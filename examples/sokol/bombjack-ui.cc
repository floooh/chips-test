//------------------------------------------------------------------------------
//  bombjack-ui.cc
//------------------------------------------------------------------------------
#include "common.h"
#include "imgui.h"
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/bombjack.h"
#define CHIPS_IMPL
#define UI_DASM_USE_Z80
#define UI_DBG_USE_Z80
#include "ui.h"
#include "util/z80dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_z80.h"
#include "ui/ui_ay38910.h"
#include "ui/ui_audio.h"
#include "ui/ui_bombjack.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

static ui_bombjack_t ui_bombjack;
static double exec_time;

void bombjackui_draw(void) {
    ui_bombjack_draw(&ui_bombjack, exec_time);
}

void bombjackui_init(bombjack_t* bj) {
    ui_init(bombjackui_draw);
    ui_bombjack_desc_t desc = {0};
    desc.sys = bj;
    desc.create_texture_cb = gfx_create_texture;
    desc.update_texture_cb = gfx_update_texture;
    desc.destroy_texture_cb = gfx_destroy_texture;
    desc.dbg_keys.break_keycode = SAPP_KEYCODE_F5;
    desc.dbg_keys.break_name = "F5";
    desc.dbg_keys.continue_keycode = SAPP_KEYCODE_F5;
    desc.dbg_keys.continue_name = "F5";
    desc.dbg_keys.step_over_keycode = SAPP_KEYCODE_F6;
    desc.dbg_keys.step_over_name = "F6";
    desc.dbg_keys.step_into_keycode = SAPP_KEYCODE_F7;
    desc.dbg_keys.step_into_name = "F7";
    desc.dbg_keys.toggle_breakpoint_keycode = SAPP_KEYCODE_F9;
    desc.dbg_keys.toggle_breakpoint_name = "F9";
    ui_bombjack_init(&ui_bombjack, &desc);
}

void bombjackui_discard(void) {
    ui_bombjack_discard(&ui_bombjack);
    ui_discard();
}

void bombjackui_exec(bombjack_t* bj, uint32_t frame_time_us) {
    const uint32_t slice_us = frame_time_us / 2;
    uint64_t start = stm_now();
    for (int i = 0; i < 2; i++) {
        /* tick the main board */
        if (ui_bombjack_before_mainboard_exec(&ui_bombjack)) {
            uint32_t ticks_to_run = clk_ticks_to_run(&bj->mainboard.clk, slice_us);
            uint32_t ticks_executed = z80_exec(&bj->mainboard.cpu, ticks_to_run);
            clk_ticks_executed(&bj->mainboard.clk, ticks_executed);
            ui_bombjack_after_mainboard_exec(&ui_bombjack);
        }
        /* tick the sound board */
        if (ui_bombjack_before_soundboard_exec(&ui_bombjack)) {
            uint32_t ticks_to_run = clk_ticks_to_run(&bj->soundboard.clk, slice_us);
            uint32_t ticks_executed = z80_exec(&bj->soundboard.cpu, ticks_to_run);
            clk_ticks_executed(&bj->soundboard.clk, ticks_executed);
            ui_bombjack_after_soundboard_exec(&ui_bombjack);
        }
    }
    bombjack_decode_video(bj);
    exec_time = stm_ms(stm_since(start));
}

} // extern "C"
