/*
    UI implementation for cpc.c, this must live in its own .cc file.
*/
#include "imgui.h"
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/i8255.h"
#include "chips/mc6845.h"
#include "chips/upd765.h"
#include "chips/crt.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/fdd.h"
#include "chips/fdd_cpc.h"
#include "systems/cpc.h"
#define CHIPS_IMPL
#include "ui.h"
#include "util/z80dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_z80.h"
#include "ui/ui_ay38910.h"
#include "ui/ui_mc6845.h"
#include "ui/ui_i8255.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
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

void cpcui_draw() {
    ui_cpc_draw(&ui_cpc, exec_time);
}

void cpcui_init(cpc_t* cpc) {
    ui_init(cpcui_draw);
    ui_cpc_desc_t desc = {0};
    desc.cpc = cpc;
    desc.boot_cb = boot_cb;
    ui_cpc_init(&ui_cpc, &desc);
}

void cpcui_discard(void) {
    ui_cpc_discard(&ui_cpc);
}

void cpcui_set_exec_time(double t) {
    exec_time = t;
}

} /* extern "C" */
