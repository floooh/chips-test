/*
    UI implementation for pengo.c, this must live in its own .cc file.
*/
#include "common.h"
#include "imgui.h"
#include "chips/z80.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/namco.h"
#define CHIPS_IMPL
#define UI_DASM_USE_Z80
#define UI_DBG_USE_Z80
#define NAMCO_PENGO
#include "ui.h"
#include "util/z80dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_z80.h"
#include "ui/ui_audio.h"
#include "ui/ui_namco.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

static ui_namco_t ui_pengo;
static double exec_time;

void pengoui_draw(void) {
    ui_namco_draw(&ui_pengo, exec_time);
}

void pengoui_init(namco_t* sys) {
    ui_init(pengoui_draw);
    ui_namco_desc_t desc = {0};
    desc.sys = sys;
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
    ui_namco_init(&ui_pengo, &desc);
}

void pengoui_discard(void) {
    ui_namco_discard(&ui_pengo);
    ui_discard();
}

void pengoui_exec(namco_t* sys, uint32_t frame_time_us) {
    if (ui_namco_before_exec(&ui_pengo)) {
        uint64_t start = stm_now();
        namco_exec(sys, frame_time_us);
        exec_time = stm_ms(stm_since(start));
        ui_namco_after_exec(&ui_pengo);
    }
    else {
        exec_time = 0.0;
    }
}

} // extern "C"

