/*
    atom-ui.c
    UI implementation for atom.c
*/
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
#include "ui.h"
#include "util/m6502dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_m6502.h"
#include "ui/ui_m6522.h"
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
    ui_atom_init(&ui_atom, &desc);
}

void atomui_discard(void) {
    ui_atom_discard(&ui_atom);
}

void atomui_set_exec_time(double t) {
    exec_time = t;
}

} /* extern "C" */