/*
    UI implementation for kc85.c, this must live in its own .cc file.
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
#include "systems/kc85.h"
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
#include "ui/ui_z80ctc.h"
#include "ui/ui_kc85sys.h"
#include "ui/ui_audio.h"
#include "ui/ui_kc85.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

extern kc85_desc_t kc85_desc(kc85_type_t type);

static double exec_time;
static ui_kc85_t ui_kc85;

/* reboot callback */
static void boot_cb(kc85_t* sys, kc85_type_t type) {
    kc85_desc_t desc = kc85_desc(type);
    kc85_init(sys, &desc);
}

void kc85ui_draw(void) {
    ui_kc85_draw(&ui_kc85, exec_time);
}

void kc85ui_init(kc85_t* kc85) {
    ui_init(kc85ui_draw);
    ui_kc85_desc_t desc = { 0 };
    desc.kc85 = kc85;
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
    ui_kc85_init(&ui_kc85, &desc);
}

void kc85ui_discard(void) {
    ui_kc85_discard(&ui_kc85);
}

void kc85ui_exec(kc85_t* kc85, uint32_t frame_time_us) {
    if (ui_kc85_before_exec(&ui_kc85)) {
        uint64_t start = stm_now();
        kc85_exec(kc85, frame_time_us);
        exec_time = stm_ms(stm_since(start));
        ui_kc85_after_exec(&ui_kc85);
    }
    else {
        exec_time = 0.0;
    }
}

} /* extern "C" */
