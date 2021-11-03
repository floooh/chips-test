/*
    UI implementation for z9001.c, this must live in its own C++ file.
*/
#include "common.h"
#include "imgui.h"
#include "chips/z80x.h"
#include "chips/z80pio.h"
#include "chips/z80ctc.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/z9001.h"
#define CHIPS_IMPL
#define UI_DASM_USE_Z80
#define UI_DBG_USE_Z80
#include "ui.h"
#include "util/z80dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_kbd.h"
#include "ui/ui_audio.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_z80.h"
#include "ui/ui_z80pio.h"
#include "ui/ui_z80ctc.h"
#include "ui/ui_z9001.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

extern z9001_desc_t z9001_desc(z9001_type_t type);

static double exec_time;
static ui_z9001_t ui_z9001;

/* reboot callback */
static void boot_cb(z9001_t* sys, z9001_type_t type) {
    z9001_desc_t desc = z9001_desc(type);
    z9001_init(sys, &desc);
}

void z9001ui_draw(void) {
    ui_z9001_draw(&ui_z9001, exec_time);
}

void z9001ui_init(z9001_t* z9001) {
    ui_init(z9001ui_draw);
    ui_z9001_desc_t desc = {0};
    desc.z9001 = z9001;
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
    ui_z9001_init(&ui_z9001, &desc);
}

void z9001ui_discard(void) {
    ui_z9001_discard(&ui_z9001);
}

void z9001ui_exec(z9001_t* z9001, uint32_t frame_time_us) {
    if (ui_z9001_before_exec(&ui_z9001)) {
        uint64_t start = stm_now();
        z9001_exec(z9001, frame_time_us);
        exec_time = stm_ms(stm_since(start));
        ui_z9001_after_exec(&ui_z9001);
    }
    else {
        exec_time = 0.0;
    }
}

} /* extern "C" */
