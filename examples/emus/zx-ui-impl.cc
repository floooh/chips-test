/*
    UI implementation for zx.c, this must live in its own C++ file.
*/
#include "chips/chips_common.h"
#include "chips/z80.h"
#include "chips/beeper.h"
#include "chips/ay38910.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/zx.h"
#define UI_DASM_USE_Z80
#define UI_DBG_USE_Z80
#define CHIPS_UTIL_IMPL
#include "util/z80dasm.h"
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
#include "ui/ui_kbd.h"
#include "ui/ui_z80.h"
#include "ui/ui_ay38910.h"
#include "ui/ui_audio.h"
#include "ui/ui_display.h"
#include "ui/ui_snapshot.h"
#include "ui/ui_zx.h"
