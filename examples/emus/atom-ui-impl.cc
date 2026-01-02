/*
    atom-ui.c
    UI implementation for atom.c
*/
#include "chips/chips_common.h"
#include "chips/m6502.h"
#include "chips/mc6847.h"
#include "chips/i8255.h"
#include "chips/m6522.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/atom.h"
#define UI_DASM_USE_M6502
#define UI_DBG_USE_M6502
#define CHIPS_UTIL_IMPL
#include "util/m6502dasm.h"
#define CHIPS_UI_IMPL
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/ui_settings.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_m6502.h"
#include "ui/ui_m6522.h"
#include "ui/ui_mc6847.h"
#include "ui/ui_i8255.h"
#include "ui/ui_audio.h"
#include "ui/ui_display.h"
#include "ui/ui_kbd.h"
#include "ui/ui_snapshot.h"
#include "ui/ui_atom.h"
