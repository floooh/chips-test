/*
   UI implementation for nes.c, this must live in a .cc file.
*/
#include "chips/chips_common.h"
#include "chips/m6502.h"
#include "chips/r2c02.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/nes.h"
#define UI_DASM_USE_M6502
#define UI_DBG_USE_M6502
#define CHIPS_UTIL_IMPL
#include "util/m6502dasm.h"
#define CHIPS_UI_IMPL
#include "imgui.h"
#include "ui/ui_audio.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_m6502.h"
#include "ui/ui_snapshot.h"
#include "ui/ui_nes.h"
