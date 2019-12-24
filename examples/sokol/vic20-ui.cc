/*
    UI implementaion for vic20.c
*/
#include "common.h"
#include "imgui.h"
#include "chips/m6502.h"
#include "chips/m6522.h"
#include "chips/m6561.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/vic20.h"
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
#include "ui/ui_m6561.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_vic20.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

vic20_desc_t vic20_desc(vic20_joystick_type_t joy_type, vic20_memory_config_t mem_config);

static ui_vic20_t ui_vic20;
static double exec_time;

/* reboot callback */
static void boot_cb(vic20_t* sys) {
    vic20_desc_t desc = vic20_desc(sys->joystick_type, sys->mem_config);
    vic20_init(sys, &desc);
}

void vic20ui_draw(void) {
    ui_vic20_draw(&ui_vic20, exec_time);
}

void vic20ui_init(vic20_t* vic20) {
    ui_init(vic20ui_draw);
    ui_vic20_desc_t desc = {0};
    desc.vic20 = vic20;
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
    ui_vic20_init(&ui_vic20, &desc);
}

void vic20ui_discard(void) {
    ui_vic20_discard(&ui_vic20);
}

void vic20ui_exec(vic20_t* vic20, uint32_t frame_time_us) {
    uint64_t start = stm_now();
    ui_vic20_exec(&ui_vic20, frame_time_us);
    exec_time = ui_vic20.dbg.dbg.stopped ? 0.0 : stm_ms(stm_since(start));
}

} /* extern "C" */
