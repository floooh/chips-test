/*
    UI implementation for kc85.c, this must live in its own .cc file.
*/
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/kc85.h"
#define UI_DASM_USE_Z80
#define UI_DBG_USE_Z80
#define CHIPS_UTIL_IMPL
#include "util/z80dasm.h"
#define CHIPS_UI_IMPL
#include "imgui.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_z80.h"
#include "ui/ui_z80pio.h"
#include "ui/ui_z80ctc.h"
#include "ui/ui_kc85sys.h"
#include "ui/ui_audio.h"
#include "ui/ui_kc85.h"
