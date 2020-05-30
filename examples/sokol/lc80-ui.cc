/*
    lc80-ui.cc
*/
#include "common.h"
#include "imgui.h"
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/lc80.h"
#define CHIPS_IMPL
#define UI_DASM_USE_Z80
#define UI_DBG_USE_Z80
#include "ui.h"
#include "util/z80dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_audio.h"
#include "ui/ui_z80.h"
#include "ui/ui_z80pio.h"
#include "ui/ui_z80ctc.h"
#include "ui/ui_lc80.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

extern lc80_desc_t lc80_desc(void);

static double exec_time;
static ui_lc80_t ui_lc80;

/* reboot callback */
static void boot_cb(lc80_t* sys) {
    lc80_desc_t desc = lc80_desc();
    lc80_init(sys, &desc);
}

void lc80ui_draw(void) {
    ui_lc80_draw(&ui_lc80, exec_time);
}

void lc80ui_init(lc80_t* sys) {
    ui_init(lc80ui_draw);
    ui_lc80_desc_t desc = { 0 };
    desc.sys = sys;
    desc.boot_cb = boot_cb;
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
    ui_lc80_init(&ui_lc80, &desc);
}

void lc80ui_discard(void) {
    ui_lc80_discard(&ui_lc80);
}

void lc80ui_exec(uint32_t frame_time_us) {
    if (ui_lc80_before_exec(&ui_lc80)) {
        uint64_t start = stm_now();
        lc80_exec(ui_lc80.sys, frame_time_us);
        exec_time = stm_ms(stm_since(start));
        ui_lc80_after_exec(&ui_lc80);
    }
    else {
        exec_time = 0.0;
    }
}

} /* extern "C" */

