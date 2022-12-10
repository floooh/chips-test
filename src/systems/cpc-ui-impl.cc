/*
    UI implementation for cpc.c, this must live in its own .cc file.
*/
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/i8255.h"
#include "chips/mc6845.h"
#include "chips/am40010.h"
#include "chips/upd765.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/fdd.h"
#include "chips/fdd_cpc.h"
#include "systems/cpc.h"
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
#include "ui/ui_ay38910.h"
#include "ui/ui_mc6845.h"
#include "ui/ui_am40010.h"
#include "ui/ui_i8255.h"
#include "ui/ui_upd765.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_fdd.h"
#include "ui/ui_cpc.h"
