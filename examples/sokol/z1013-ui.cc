/*
    UI implementation for z1013.c, this must live in its own C++ file.
*/
#include "common.h"
#include "imgui.h"
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/z1013.h"
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
#include "ui/ui_z80pio.h"
#include "ui/ui_z1013.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

extern z1013_desc_t z1013_desc(z1013_type_t type);

static double exec_time;
static ui_z1013_t ui_z1013;

/* reboot callback */
static void boot_cb(z1013_t* sys, z1013_type_t type) {
    z1013_desc_t desc = z1013_desc(type);
    z1013_init(sys, &desc);
}

void z1013ui_draw(void) {
    ui_z1013_draw(&ui_z1013, exec_time);
}

void z1013ui_init(z1013_t* z1013) {
    ui_init(z1013ui_draw);
    ui_z1013_desc_t desc = {0};
    desc.z1013 = z1013;
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
    ui_z1013_init(&ui_z1013, &desc);
}

void z1013ui_discard(void) {
    ui_z1013_discard(&ui_z1013);
}

void z1013ui_exec(z1013_t* z1013, uint32_t frame_time_us) {
    if (ui_z1013_before_exec(&ui_z1013)) {
        uint64_t start = stm_now();
        z1013_exec(z1013, frame_time_us);
        exec_time = stm_ms(stm_since(start));
        ui_z1013_after_exec(&ui_z1013);
    }
    else {
        exec_time = 0.0;
    }
}

} /* extern "C" */
