/*
    UI implementation for cpc.c, this must live in its own .cc file.
*/
#include "common.h"
#include "imgui.h"
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/i8255.h"
#include "chips/mc6845.h"
#include "chips/am40010.h"
#include "chips/upd765.h"
#include "chips/crt.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/fdd.h"
#include "chips/fdd_cpc.h"
#include "systems/cpc.h"
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
#include "ui/ui_mc6845.h"
#include "ui/ui_am40010.h"
#include "ui/ui_i8255.h"
#include "ui/ui_upd765.h"
#include "ui/ui_cpc_ga.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_fdd.h"
#include "ui/ui_cpc.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

cpc_desc_t cpc_desc(cpc_type_t type, cpc_joystick_type_t joy_type);

static ui_cpc_t ui_cpc;
static double exec_time;

/* reboot callback */
static void boot_cb(cpc_t* sys, cpc_type_t type) {
    cpc_desc_t desc = cpc_desc(type, sys->joystick_type);
    cpc_init(sys, &desc);
}

void cpcui_draw(void) {
    ui_cpc_draw(&ui_cpc, exec_time);
}

void cpcui_init(cpc_t* cpc) {
    ui_init(cpcui_draw);
    ui_cpc_desc_t desc = {0};
    desc.cpc = cpc;
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
    ui_cpc_init(&ui_cpc, &desc);
}

void cpcui_discard(void) {
    ui_cpc_discard(&ui_cpc);
}

void cpcui_exec(cpc_t* cpc, uint32_t frame_time_us) {
    if (ui_cpc_before_exec(&ui_cpc)) {
        uint64_t start = stm_now();
        cpc_exec(cpc, frame_time_us);
        exec_time = stm_ms(stm_since(start));
        ui_cpc_after_exec(&ui_cpc);
    }
    else {
        exec_time = 0.0;
    }
}

} /* extern "C" */
