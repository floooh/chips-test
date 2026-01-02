/*
    UI implementation for c64.c, this must live in a .cc file.
*/
#include "chips/chips_common.h"
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c1530.h"
#include "chips/m6522.h"
#include "systems/c1541.h"
#include "systems/c64.h"
#define UI_DASM_USE_M6502
#define UI_DBG_USE_M6502
#define CHIPS_UTIL_IMPL
#include "util/m6502dasm.h"
#define CHIPS_UI_IMPL
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/ui_util.h"
#include "ui/ui_settings.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_m6502.h"
#include "ui/ui_m6526.h"
#include "ui/ui_m6581.h"
#include "ui/ui_m6569.h"
#include "ui/ui_audio.h"
#include "ui/ui_display.h"
#include "ui/ui_kbd.h"
#include "ui/ui_snapshot.h"
#include "ui/ui_c64.h"
