/*
    UI implementation for pacman.c, this must live in its own .cc file.
*/
#include "common.h"
#include "imgui.h"
#include "chips/z80x.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/namco.h"
#define UI_DASM_USE_Z80
#define UI_DBG_USE_Z80
#define CHIPS_UTIL_IMPL
#include "util/z80dasm.h"
#define CHIPS_UI_IMPL
#define NAMCO_PACMAN
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_z80.h"
#include "ui/ui_audio.h"
#include "ui/ui_namco.h"
