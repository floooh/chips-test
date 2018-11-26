/*
    UI implementation for kc85.c, this must live in its own .cc file.
*/
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
#include "ui.h"
#include "util/z80dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
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

void kc85ui_draw() {
    ui_kc85_draw(&ui_kc85, exec_time);
}

void kc85ui_init(kc85_t* kc85) {
    ui_init(kc85ui_draw);
    ui_kc85_desc_t desc = { 0 };
    desc.kc85 = kc85;
    desc.boot_cb = boot_cb;
    ui_kc85_init(&ui_kc85, &desc);
}

void kc85ui_discard(void) {
    ui_kc85_discard(&ui_kc85);
}

void kc85ui_set_exec_time(double t) {
    exec_time = t;
}

} /* extern "C" */
