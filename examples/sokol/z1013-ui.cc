/*
    UI implementation for z1013.c, this must live in its own C++ file.
*/
#include "imgui.h"
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/z1013.h"
#define CHIPS_IMPL
#include "ui.h"
#include "util/z80dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
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

void z1013ui_draw() {
    ui_z1013_draw(&ui_z1013, exec_time);
}

void z1013ui_init(z1013_t* z1013) {
    ui_init(z1013ui_draw);
    ui_z1013_desc_t desc = {0};
    desc.z1013 = z1013;
    desc.boot_cb = boot_cb;
    ui_z1013_init(&ui_z1013, &desc);
}

void z1013ui_discard(void) {
    ui_z1013_discard(&ui_z1013);
}

void z1013ui_set_exec_time(double t) {
    exec_time = t;
}

} /* extern "C" */
