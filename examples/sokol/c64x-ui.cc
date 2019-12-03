/*
    UI implementation for c64.c, this must live in a .cc file.
*/
#include "common.h"
#include "imgui.h"
#include "chips/m6502x.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c64x.h"
#define CHIPS_IMPL
#define UI_DASM_USE_M6502
#define UI_DBG_USE_M6502X
#include "ui.h"
#include "util/m6502dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_m6502x.h"
#include "ui/ui_m6526.h"
#include "ui/ui_m6581.h"
#include "ui/ui_m6569.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_c64x.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

c64_desc_t c64_desc(c64_joystick_type_t joy_type);

static ui_c64x_t ui_c64;
static double exec_time;

/* reboot callback */
static void boot_cb(c64_t* sys) {
    c64_desc_t desc = c64_desc(sys->joystick_type);
    c64_init(sys, &desc);
}

void c64ui_draw(void) {
    ui_c64x_draw(&ui_c64, exec_time);
}

void c64ui_init(c64_t* c64) {
    ui_init(c64ui_draw);
    ui_c64x_desc_t desc = {0};
    desc.c64 = c64;
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
    ui_c64x_init(&ui_c64, &desc);
}

void c64ui_discard(void) {
    ui_c64x_discard(&ui_c64);
}

void c64ui_exec(c64_t* c64, uint32_t frame_time_us) {
    uint64_t start = stm_now();
    ui_c64x_exec(&ui_c64, frame_time_us);
    exec_time = ui_c64.dbg.dbg.stopped ? 0.0 : stm_ms(stm_since(start));
}

} /* extern "C" */
