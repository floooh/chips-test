/*
    UI implementation for zx.c, this must live in its own C++ file.
*/
#include "common.h"
#include "imgui.h"
#include "chips/z80.h"
#include "chips/beeper.h"
#include "chips/ay38910.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/zx.h"
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
#include "ui/ui_kbd.h"
#include "ui/ui_z80.h"
#include "ui/ui_ay38910.h"
#include "ui/ui_audio.h"
#include "ui/ui_zx.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

extern zx_desc_t zx_desc(zx_type_t type, zx_joystick_type_t joy_type);

static double exec_time;
static ui_zx_t ui_zx;

/* reboot callback */
static void boot_cb(zx_t* sys, zx_type_t type) {
    zx_desc_t desc = zx_desc(type, sys->joystick_type);
    zx_init(sys, &desc);
}

void zxui_draw(void) {
    ui_zx_draw(&ui_zx, exec_time);
}

void zxui_init(zx_t* zx) {
    ui_init(zxui_draw);
    ui_zx_desc_t desc = {0};
    desc.zx = zx;
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
    ui_zx_init(&ui_zx, &desc);
}

void zxui_discard(void) {
    ui_zx_discard(&ui_zx);
}

void zxui_exec(zx_t* zx, uint32_t frame_time_us) {
    if (ui_zx_before_exec(&ui_zx)) {
        uint64_t start = stm_now();
        zx_exec(zx, frame_time_us);
        exec_time = stm_ms(stm_since(start));
        ui_zx_after_exec(&ui_zx);
    }
    else {
        exec_time = 0.0;
    }
}

} /* extern "C" */
