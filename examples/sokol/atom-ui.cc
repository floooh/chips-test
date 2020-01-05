/*
    atom-ui.c
    UI implementation for atom.c
*/
#include "common.h"
#include "imgui.h"
#include "chips/m6502.h"
#include "chips/mc6847.h"
#include "chips/i8255.h"
#include "chips/m6522.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/atom.h"
#define CHIPS_IMPL
#define UI_DASM_USE_M6502
#define UI_DBG_USE_M6502
#include "ui.h"
#include "util/m6502dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_m6502.h"
#include "ui/ui_m6522.h"
#include "ui/ui_mc6847.h"
#include "ui/ui_i8255.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_atom.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

atom_desc_t atom_desc(atom_joystick_type_t joy_type);

static ui_atom_t ui_atom;
static double exec_time;

/* reboot callback */
static void boot_cb(atom_t* sys) {
    atom_desc_t desc = atom_desc(sys->joystick_type);
    atom_init(sys, &desc);
}

void atomui_draw(void) {
    ui_atom_draw(&ui_atom, exec_time);
}

void atomui_init(atom_t* atom) {
    ui_init(atomui_draw);
    ui_atom_desc_t desc = {0};
    desc.atom = atom;
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
    desc.dbg_keys.step_tick_keycode = SAPP_KEYCODE_F8;
    desc.dbg_keys.step_tick_name = "F8";
    desc.dbg_keys.toggle_breakpoint_keycode = SAPP_KEYCODE_F9;
    desc.dbg_keys.toggle_breakpoint_name = "F9";
    ui_atom_init(&ui_atom, &desc);
}

void atomui_discard(void) {
    ui_atom_discard(&ui_atom);
}

void atomui_exec(atom_t* atom, uint32_t frame_time_us) {
    uint64_t start = stm_now();
    ui_atom_exec(&ui_atom, frame_time_us);
    exec_time = ui_atom.dbg.dbg.stopped ? 0.0 : stm_ms(stm_since(start));
}

} /* extern "C" */
