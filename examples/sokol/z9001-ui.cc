/*
    UI implementation for z9001.c, this must live in its own C++ file.
*/
#include "imgui.h"
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/z80ctc.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/z9001.h"
#define CHIPS_IMPL
#include "ui.h"
#include "util/z80dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_kbd.h"
#include "ui/ui_audio.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
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

void z9001ui_draw() {
    ui_z9001_draw(&ui_z9001, exec_time);
}

void z9001ui_init(z9001_t* z9001) {
    ui_init(z9001ui_draw);
    ui_z9001_desc_t desc = {0};
    desc.z9001 = z9001;
    desc.boot_cb = boot_cb;
    ui_z9001_init(&ui_z9001, &desc);
}

void z9001ui_discard(void) {
    ui_z9001_discard(&ui_z9001);
}

void z9001ui_set_exec_time(double t) {
    exec_time = t;
}

} /* extern "C" */
