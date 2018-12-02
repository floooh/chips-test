/*
    UI implementation for c64.c, this must live in a .cc file.
*/
#include "imgui.h"
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c64.h"
#define CHIPS_IMPL
#define UI_DASM_USE_M6502
#include "ui.h"
#include "util/m6502dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_m6502.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_c64.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

c64_desc_t c64_desc(c64_joystick_type_t joy_type);

static ui_c64_t ui_c64;
static double exec_time;

/* reboot callback */
static void boot_cb(c64_t* sys) {
    c64_desc_t desc = c64_desc(sys->joystick_type);
    c64_init(sys, &desc);
}

void c64ui_draw(void) {
    ui_c64_draw(&ui_c64, exec_time);
}

void c64ui_init(c64_t* c64) {
    ui_init(c64ui_draw);
    ui_c64_desc_t desc = {0};
    desc.c64 = c64;
    desc.boot_cb = boot_cb;
    ui_c64_init(&ui_c64, &desc);
}

void c64ui_discard(void) {
    ui_c64_discard(&ui_c64);
}

void c64ui_set_exec_time(double t) {
    exec_time = t;
}

} /* extern "C" */
