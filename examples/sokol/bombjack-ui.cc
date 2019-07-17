//------------------------------------------------------------------------------
//  bombjack-ui.cc
//------------------------------------------------------------------------------
#include "common.h"
#include "imgui.h"
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/bombjack.h"
#define CHIPS_IMPL
#define UI_DASM_USE_Z80
#define UI_DBG_USE_Z80
#include "ui.h"
#include "util/z80dasm.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_z80.h"
#include "ui/ui_ay38910.h"
#include "ui/ui_audio.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

extern "C" {

static struct {
    bombjack_t* bj;
    struct {
        ui_z80_t cpu;
        ui_dbg_t dbg;
    } main;
    struct {
        ui_z80_t cpu;
        ui_ay38910_t psg[3];
        ui_audio_t audio;
        ui_dbg_t dbg;
    } sound;
    ui_memmap_t memmap;
    ui_memedit_t memedit[4];
    ui_dasm_t dasm[4];
    double exec_time;
} ui;

static const ui_chip_pin_t _ui_bombjack_cpu_pins[] = {
    { "D0",     0,      Z80_D0 },
    { "D1",     1,      Z80_D1 },
    { "D2",     2,      Z80_D2 },
    { "D3",     3,      Z80_D3 },
    { "D4",     4,      Z80_D4 },
    { "D5",     5,      Z80_D5 },
    { "D6",     6,      Z80_D6 },
    { "D7",     7,      Z80_D7 },
    { "M1",     9,      Z80_M1 },
    { "MREQ",   10,     Z80_MREQ },
    { "IORQ",   11,     Z80_IORQ },
    { "RD",     12,     Z80_RD },
    { "WR",     13,     Z80_WR },
    { "HALT",   14,     Z80_HALT },
    { "INT",    15,     Z80_INT },
    { "NMI",    16,     Z80_NMI },
    { "WAIT",   17,     Z80_WAIT_MASK },
    { "A0",     18,     Z80_A0 },
    { "A1",     19,     Z80_A1 },
    { "A2",     20,     Z80_A2 },
    { "A3",     21,     Z80_A3 },
    { "A4",     22,     Z80_A4 },
    { "A5",     23,     Z80_A5 },
    { "A6",     24,     Z80_A6 },
    { "A7",     25,     Z80_A7 },
    { "A8",     26,     Z80_A8 },
    { "A9",     27,     Z80_A9 },
    { "A10",    28,     Z80_A10 },
    { "A11",    29,     Z80_A11 },
    { "A12",    30,     Z80_A12 },
    { "A13",    31,     Z80_A13 },
    { "A14",    32,     Z80_A14 },
    { "A15",    33,     Z80_A15 },
};

static const ui_chip_pin_t _ui_bombjack_psg_pins[] = {
    { "DA0",  0, AY38910_DA0 },
    { "DA1",  1, AY38910_DA1 },
    { "DA2",  2, AY38910_DA2 },
    { "DA3",  3, AY38910_DA3 },
    { "DA4",  4, AY38910_DA4 },
    { "DA5",  5, AY38910_DA5 },
    { "DA6",  6, AY38910_DA6 },
    { "DA7",  7, AY38910_DA7 },
    { "BDIR", 9, AY38910_BDIR },
    { "BC1",  10, AY38910_BC1 },
    { "IOA0", 11, AY38910_IOA0 },
    { "IOA1", 12, AY38910_IOA1 },
    { "IOA2", 13, AY38910_IOA2 },
    { "IOA3", 14, AY38910_IOA3 },
    { "IOA4", 15, AY38910_IOA4 },
    { "IOA5", 16, AY38910_IOA5 },
    { "IOA6", 17, AY38910_IOA6 },
    { "IOA7", 18, AY38910_IOA7 },
};

#define MEMLAYER_MAIN (0)
#define MEMLAYER_SOUND (1)
#define MEMLAYER_CHARS (2)
#define MEMLAYER_TILES (3)
#define MEMLAYER_SPRITES (4)
#define MEMLAYER_MAPS (5)
#define NUM_MEMLAYERS (6)
static const char* memlayer_names[NUM_MEMLAYERS] = {
    "Main", "Sound", "Chars", "Tiles", "Sprites", "Maps"
};


static uint8_t _ui_bombjack_mem_read(int layer, uint16_t addr, void* /*user_data*/) {
    switch (layer) {
        case MEMLAYER_MAIN:
            return mem_rd(&ui.bj->mainboard.mem, addr);
        case MEMLAYER_SOUND:
            return mem_rd(&ui.bj->soundboard.mem, addr);
        case MEMLAYER_CHARS:
            return (addr < 0x3000) ? ui.bj->rom_chars[addr/0x1000][addr&0x0FFF] : 0xFF;
        case MEMLAYER_TILES:
            return (addr < 0x6000) ? ui.bj->rom_tiles[addr/0x2000][addr&0x1FFF] : 0xFF;
        case MEMLAYER_SPRITES:
            return (addr < 0x6000) ? ui.bj->rom_sprites[addr/0x2000][addr&0x1FFF] : 0xFF;
        case MEMLAYER_MAPS:
            return (addr < 0x1000) ? ui.bj->rom_maps[addr/0x1000][addr&0x0FFF] : 0xFF;
        default:
            return 0xFF;
    }
}

static void _ui_bombjack_mem_write(int layer, uint16_t addr, uint8_t data, void* /*user_data*/) {
    switch (layer) {
        case MEMLAYER_MAIN:
            mem_wr(&ui.bj->mainboard.mem, addr, data);
            break;
        case MEMLAYER_SOUND:
            mem_wr(&ui.bj->soundboard.mem, addr, data);
            break;
        case MEMLAYER_CHARS:
            if (addr < 0x3000) {
                ui.bj->rom_chars[addr/0x1000][addr&0x0FFF] = data;
            }
            break;
        case MEMLAYER_TILES:
            if (addr < 0x6000) {
                ui.bj->rom_tiles[addr/0x2000][addr&0x1FFF] = data;
            }
            break;
        case MEMLAYER_SPRITES:
            if (addr < 0x6000) {
                ui.bj->rom_sprites[addr/0x2000][addr&0x1FFF] = data;
            }
            break;
        case MEMLAYER_MAPS:
            if (addr < 0x1000) {
                ui.bj->rom_maps[0][addr] = data;
            }
            break;
    }
}

static bool test_bits(uint8_t val, uint8_t mask, uint8_t bits) {
    return bits == (val & mask);
}

static uint8_t set_bits(uint8_t val, uint8_t mask, uint8_t bits) {
    return (val & ~mask) | bits;
}

static uint8_t toggle_bits(uint8_t val, uint8_t mask, uint8_t bits) {
    return (val & ~mask) ^ bits;
}

static bool test_dsw1(uint8_t mask, uint8_t bits) {
    return test_bits(ui.bj->mainboard.dsw1, mask, bits);
}

static void set_dsw1(uint8_t mask, uint8_t bits) {
    ui.bj->mainboard.dsw1 = set_bits(ui.bj->mainboard.dsw1, mask, bits);
}

static void toggle_dsw1(uint8_t mask, uint8_t bits) {
    ui.bj->mainboard.dsw1 = toggle_bits(ui.bj->mainboard.dsw1, mask, bits);
}

static bool test_dsw2(uint8_t mask, uint8_t bits) {
    return test_bits(ui.bj->mainboard.dsw2, mask, bits);
}

static void set_dsw2(uint8_t mask, uint8_t bits) {
    ui.bj->mainboard.dsw2 = set_bits(ui.bj->mainboard.dsw2, mask, bits);
}

void bombjackui_draw(void) {
    ui.bj->mainboard.sys = 0;
    ui_memmap_draw(&ui.memmap);
    ui_dbg_draw(&ui.main.dbg);
    ui_dbg_draw(&ui.sound.dbg);
    ui_z80_draw(&ui.main.cpu);
    ui_z80_draw(&ui.sound.cpu);
    for (int i = 0; i < 3; i++) {
        ui_ay38910_draw(&ui.sound.psg[i]);
    }
    for (int i = 0; i < 4; i++) {
        ui_memedit_draw(&ui.memedit[i]);
        ui_dasm_draw(&ui.dasm[i]);
    }
    ui_audio_draw(&ui.sound.audio, ui.bj->sample_pos);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("System")) {
            if (ImGui::MenuItem("Reboot")) {
                bombjack_reset(ui.bj);
                ui_dbg_reboot(&ui.main.dbg);
                ui_dbg_reboot(&ui.sound.dbg);
            }
            if (ImGui::BeginMenu("Player 1")) {
                if (ImGui::MenuItem("Insert Coin")) {
                    ui.bj->mainboard.sys |= BOMBJACK_SYS_P1_COIN;
                }
                if (ImGui::BeginMenu("1 Coin gives:")) {
                    if (ImGui::MenuItem("1 Credit", nullptr, test_dsw1(BOMBJACK_DSW1_P1_MASK, BOMBJACK_DSW1_P1_1COIN_1PLAY))) {
                        set_dsw1(BOMBJACK_DSW1_P1_MASK, BOMBJACK_DSW1_P1_1COIN_1PLAY);
                    }
                    if (ImGui::MenuItem("2 Credits", nullptr, test_dsw1(BOMBJACK_DSW1_P1_MASK, BOMBJACK_DSW1_P1_1COIN_2PLAY))) {
                        set_dsw1(BOMBJACK_DSW1_P1_MASK, BOMBJACK_DSW1_P1_1COIN_2PLAY);
                    }
                    if (ImGui::MenuItem("3 Credits", nullptr, test_dsw1(BOMBJACK_DSW1_P1_MASK, BOMBJACK_DSW1_P1_1COIN_3PLAY))) {
                        set_dsw1(BOMBJACK_DSW1_P1_MASK, BOMBJACK_DSW1_P1_1COIN_3PLAY);
                    }
                    if (ImGui::MenuItem("6 Credits", nullptr, test_dsw1(BOMBJACK_DSW1_P1_MASK, BOMBJACK_DSW1_P1_1COIN_5PLAY))) {
                        set_dsw1(BOMBJACK_DSW1_P1_MASK, BOMBJACK_DSW1_P1_1COIN_5PLAY);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Play Button")) {
                    ui.bj->mainboard.sys |= BOMBJACK_SYS_P1_START;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Player 2")) {
                if (ImGui::MenuItem("Insert Coin")) {
                    ui.bj->mainboard.sys |= BOMBJACK_SYS_P2_COIN;
                }
                if (ImGui::BeginMenu("1 Coin gives:")) {
                    if (ImGui::MenuItem("1 Credit", nullptr, test_dsw1(BOMBJACK_DSW1_P2_MASK, BOMBJACK_DSW1_P2_1COIN_1PLAY))) {
                        set_dsw1(BOMBJACK_DSW1_P2_MASK, BOMBJACK_DSW1_P2_1COIN_1PLAY);
                    }
                    if (ImGui::MenuItem("2 Credits", nullptr, test_dsw1(BOMBJACK_DSW1_P2_MASK, BOMBJACK_DSW1_P2_1COIN_2PLAY))) {
                        set_dsw1(BOMBJACK_DSW1_P2_MASK, BOMBJACK_DSW1_P2_1COIN_2PLAY);
                    }
                    if (ImGui::MenuItem("3 Credits", nullptr, test_dsw1(BOMBJACK_DSW1_P2_MASK, BOMBJACK_DSW1_P2_1COIN_3PLAY))) {
                        set_dsw1(BOMBJACK_DSW1_P2_MASK, BOMBJACK_DSW1_P2_1COIN_3PLAY);
                    }
                    if (ImGui::MenuItem("6 Credits", nullptr, test_dsw1(BOMBJACK_DSW1_P2_MASK, BOMBJACK_DSW1_P2_1COIN_5PLAY))) {
                        set_dsw1(BOMBJACK_DSW1_P2_MASK, BOMBJACK_DSW1_P2_1COIN_5PLAY);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Play Button")) {
                    ui.bj->mainboard.sys |= BOMBJACK_SYS_P2_START;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Lives")) {
                if (ImGui::MenuItem("2", nullptr, test_dsw1(BOMBJACK_DSW1_JACKS_MASK, BOMBJACK_DSW1_JACKS_2))) {
                    set_dsw1(BOMBJACK_DSW1_JACKS_MASK, BOMBJACK_DSW1_JACKS_2);
                }
                if (ImGui::MenuItem("3", nullptr, test_dsw1(BOMBJACK_DSW1_JACKS_MASK, BOMBJACK_DSW1_JACKS_3))) {
                    set_dsw1(BOMBJACK_DSW1_JACKS_MASK, BOMBJACK_DSW1_JACKS_3);
                }
                if (ImGui::MenuItem("4", nullptr, test_dsw1(BOMBJACK_DSW1_JACKS_MASK, BOMBJACK_DSW1_JACKS_4))) {
                    set_dsw1(BOMBJACK_DSW1_JACKS_MASK, BOMBJACK_DSW1_JACKS_4);
                }
                if (ImGui::MenuItem("5", nullptr, test_dsw1(BOMBJACK_DSW1_JACKS_MASK, BOMBJACK_DSW1_JACKS_5))) {
                    set_dsw1(BOMBJACK_DSW1_JACKS_MASK, BOMBJACK_DSW1_JACKS_5);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Cabinet")) {
                if (ImGui::MenuItem("Upright", nullptr, test_dsw1(BOMBJACK_DSW1_CABINET_MASK, BOMBJACK_DSW1_CABINET_UPRIGHT))) {
                    set_dsw1(BOMBJACK_DSW1_CABINET_MASK, BOMBJACK_DSW1_CABINET_UPRIGHT);
                }
                if (ImGui::MenuItem("Cocktail (not impl)", nullptr, test_dsw1(BOMBJACK_DSW1_CABINET_MASK, BOMBJACK_DSW1_CABINET_COCKTAIL))) {
                    set_dsw1(BOMBJACK_DSW1_CABINET_MASK, BOMBJACK_DSW1_CABINET_COCKTAIL);
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Demo Sounds", nullptr, test_dsw1(BOMBJACK_DSW1_DEMOSOUND_MASK, BOMBJACK_DSW1_DEMOSOUND_ON))) {
                toggle_dsw1(BOMBJACK_DSW1_DEMOSOUND_MASK, BOMBJACK_DSW1_DEMOSOUND_ON);
            }
            if (ImGui::BeginMenu("Bird Speed")) {
                if (ImGui::MenuItem("Easy", nullptr, test_dsw2(BOMBJACK_DSW2_BIRDSPEED_MASK, BOMBJACK_DSW2_BIRDSPEED_EASY))) {
                    set_dsw2(BOMBJACK_DSW2_BIRDSPEED_MASK, BOMBJACK_DSW2_BIRDSPEED_EASY);
                }
                if (ImGui::MenuItem("Medium", nullptr, test_dsw2(BOMBJACK_DSW2_BIRDSPEED_MASK, BOMBJACK_DSW2_BIRDSPEED_MODERATE))) {
                    set_dsw2(BOMBJACK_DSW2_BIRDSPEED_MASK, BOMBJACK_DSW2_BIRDSPEED_MODERATE);
                }
                if (ImGui::MenuItem("Hard", nullptr, test_dsw2(BOMBJACK_DSW2_BIRDSPEED_MASK, BOMBJACK_DSW2_BIRDSPEED_HARD))) {
                    set_dsw2(BOMBJACK_DSW2_BIRDSPEED_MASK, BOMBJACK_DSW2_BIRDSPEED_HARD);
                }
                if (ImGui::MenuItem("Hardest", nullptr, test_dsw2(BOMBJACK_DSW2_BIRDSPEED_MASK, BOMBJACK_DSW2_BIRDSPEED_HARDER))) {
                    set_dsw2(BOMBJACK_DSW2_BIRDSPEED_MASK, BOMBJACK_DSW2_BIRDSPEED_HARDER);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Enemies Number & Speed")) {
                if (ImGui::MenuItem("Easy", nullptr, test_dsw2(BOMBJACK_DSW2_DIFFICULTY_MASK, BOMBJACK_DSW2_DIFFICULTY_EASY))) {
                    set_dsw2(BOMBJACK_DSW2_DIFFICULTY_MASK, BOMBJACK_DSW2_DIFFICULTY_EASY);
                }
                if (ImGui::MenuItem("Medium", nullptr, test_dsw2(BOMBJACK_DSW2_DIFFICULTY_MASK, BOMBJACK_DSW2_DIFFICULTY_MODERATE))) {
                    set_dsw2(BOMBJACK_DSW2_DIFFICULTY_MASK, BOMBJACK_DSW2_DIFFICULTY_MODERATE);
                }
                if (ImGui::MenuItem("Hard", nullptr, test_dsw2(BOMBJACK_DSW2_DIFFICULTY_MASK, BOMBJACK_DSW2_DIFFICULTY_HARD))) {
                    set_dsw2(BOMBJACK_DSW2_DIFFICULTY_MASK, BOMBJACK_DSW2_DIFFICULTY_HARD);
                }
                if (ImGui::MenuItem("Hardest", nullptr, test_dsw2(BOMBJACK_DSW2_DIFFICULTY_MASK, BOMBJACK_DSW2_DIFFICULTY_HARDER))) {
                    set_dsw2(BOMBJACK_DSW2_DIFFICULTY_MASK, BOMBJACK_DSW2_DIFFICULTY_HARDER);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Special Coin")) {
                if (ImGui::MenuItem("Easy", nullptr, test_dsw2(BOMBJACK_DSW2_SPECIALCOIN_MASK, BOMBJACK_DSW2_SPECIALCOIN_EASY))) {
                    set_dsw2(BOMBJACK_DSW2_SPECIALCOIN_MASK, BOMBJACK_DSW2_SPECIALCOIN_EASY);
                }
                if (ImGui::MenuItem("Hard", nullptr, test_dsw2(BOMBJACK_DSW2_SPECIALCOIN_MASK, BOMBJACK_DSW2_SPECIALCOIN_HARD))) {
                    set_dsw2(BOMBJACK_DSW2_SPECIALCOIN_MASK, BOMBJACK_DSW2_SPECIALCOIN_HARD);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Hardware")) {
            ImGui::MenuItem("Memory Map", 0, &ui.memmap.open);
            ImGui::MenuItem("Z80 (Main Board)", 0, &ui.main.cpu.open);
            ImGui::MenuItem("Z80 (Sound Board)", 0, &ui.sound.cpu.open);
            ImGui::MenuItem("AY-3-8912 #1", 0, &ui.sound.psg[0].open);
            ImGui::MenuItem("AY-3-8912 #2", 0, &ui.sound.psg[1].open);
            ImGui::MenuItem("AY-3-8912 #3", 0, &ui.sound.psg[2].open);
            ImGui::MenuItem("Audio Output", 0, &ui.sound.audio.open);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            if (ImGui::BeginMenu("Main Board")) {
                ImGui::MenuItem("CPU Debugger", 0, &ui.main.dbg.ui.open);
                ImGui::MenuItem("Breakpoints", 0, &ui.main.dbg.ui.show_breakpoints);
                ImGui::MenuItem("Memory Heatmap", 0, &ui.main.dbg.ui.show_heatmap);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Sound Board")) {
                ImGui::MenuItem("CPU Debugger", 0, &ui.sound.dbg.ui.open);
                ImGui::MenuItem("Breakpoints", 0, &ui.sound.dbg.ui.show_breakpoints);
                ImGui::MenuItem("Memory Heatmap", 0, &ui.sound.dbg.ui.show_heatmap);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Memory Editor")) {
                ImGui::MenuItem("Window #1", 0, &ui.memedit[0].open);
                ImGui::MenuItem("Window #2", 0, &ui.memedit[1].open);
                ImGui::MenuItem("Window #3", 0, &ui.memedit[2].open);
                ImGui::MenuItem("Window #4", 0, &ui.memedit[3].open);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Disassembler")) {
                ImGui::MenuItem("Window #1", 0, &ui.dasm[0].open);
                ImGui::MenuItem("Window #2", 0, &ui.dasm[1].open);
                ImGui::MenuItem("Window #3", 0, &ui.dasm[2].open);
                ImGui::MenuItem("Window #4", 0, &ui.dasm[3].open);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ui_util_options_menu(ui.exec_time, ui.main.dbg.dbg.stopped);
        ImGui::EndMainMenuBar();
    }
}

void bombjackui_init(bombjack_t* bj) {
    memset(&ui, 0, sizeof(ui));
    ui.bj = bj;
    ui_init(bombjackui_draw);

    int x = 20, y = 20, dx = 10, dy = 10;
    {
        ui_dbg_desc_t desc = {0};
        desc.title = "CPU Debugger (Main)";
        desc.x = x;
        desc.y = y;
        desc.z80 = &ui.bj->mainboard.cpu;
        desc.read_cb = _ui_bombjack_mem_read;
        desc.read_layer = MEMLAYER_MAIN;
        desc.create_texture_cb = gfx_create_texture;
        desc.update_texture_cb = gfx_update_texture;
        desc.destroy_texture_cb = gfx_destroy_texture;
        desc.keys.break_keycode = SAPP_KEYCODE_F5;
        desc.keys.break_name = "F5";
        desc.keys.continue_keycode = SAPP_KEYCODE_F5;
        desc.keys.continue_name = "F5";
        desc.keys.step_over_keycode = SAPP_KEYCODE_F6;
        desc.keys.step_over_name = "F6";
        desc.keys.step_into_keycode = SAPP_KEYCODE_F7;
        desc.keys.step_into_name = "F7";
        desc.keys.toggle_breakpoint_keycode = SAPP_KEYCODE_F9;
        desc.keys.toggle_breakpoint_name = "F9";
        ui_dbg_init(&ui.main.dbg, &desc);
        x += dx; desc.x = x;
        y += dy; desc.y = y;
        desc.title = "CPU Debugger (Sound)";
        desc.z80 = &ui.bj->soundboard.cpu;
        desc.read_layer = MEMLAYER_SOUND;
        ui_dbg_init(&ui.sound.dbg, &desc);
    }
    x += dx; y += dy;
    {
        ui_z80_desc_t desc = {0};
        desc.title = "Z80 CPU (Main)";
        desc.cpu = &bj->mainboard.cpu;
        desc.x = x;
        desc.y = y;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "Z80\nCPU", 36, _ui_bombjack_cpu_pins);
        ui_z80_init(&ui.main.cpu, &desc);
    }
    x += dx; y += dy;
    {
        ui_z80_desc_t desc = {0};
        desc.title = "Z80 CPU (Sound)";
        desc.cpu = &bj->soundboard.cpu;
        desc.x = x;
        desc.y = y;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "Z80\nCPU", 36, _ui_bombjack_cpu_pins);
        ui_z80_init(&ui.sound.cpu, &desc);
    }
    for (int i = 0; i < 3; i++) {
        x += dx; y += dy;
        ui_ay38910_desc_t desc = {0};
        switch (i) {
            case 0: desc.title = "AY-3-8910 (0)"; break;
            case 1: desc.title = "AY-3-8910 (1)"; break;
            case 2: desc.title = "AY-3-8910 (2)"; break;
        }
        desc.ay = &bj->soundboard.psg[i];
        desc.x = x;
        desc.y = y;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "8910", 22, _ui_bombjack_psg_pins);
        ui_ay38910_init(&ui.sound.psg[i], &desc);
    }
    x += dx; y += dy;
    {
        ui_audio_desc_t desc = {0};
        desc.title = "Audio Output";
        desc.sample_buffer = ui.bj->sample_buffer;
        desc.num_samples = ui.bj->num_samples,
        desc.x = x;
        desc.y = y;
        ui_audio_init(&ui.sound.audio, &desc);
    }
    x += dx; y += dy;
    {
        ui_memmap_desc_t desc = {0};
        desc.title = "Memory Map (Main)";
        desc.x = x;
        desc.y = y;
        ui_memmap_init(&ui.memmap, &desc);
        ui_memmap_layer(&ui.memmap, "Main");
            ui_memmap_region(&ui.memmap, "ROM 0", 0x0000, 0x2000, true);
            ui_memmap_region(&ui.memmap, "ROM 1", 0x2000, 0x2000, true);
            ui_memmap_region(&ui.memmap, "ROM 2", 0x4000, 0x2000, true);
            ui_memmap_region(&ui.memmap, "ROM 4", 0x6000, 0x2000, true);
            ui_memmap_region(&ui.memmap, "RAM", 0x8000, 0x1000, true);
            ui_memmap_region(&ui.memmap, "Video RAM", 0x9000, 0x0400, true);
            ui_memmap_region(&ui.memmap, "Color RAM", 0x9400, 0x0400, true);
            ui_memmap_region(&ui.memmap, "Sprites", 0x9820, 0x0060, true);
            ui_memmap_region(&ui.memmap, "Palette", 0x9C00, 0x0100, true);
            ui_memmap_region(&ui.memmap, "ROM 5", 0xC000, 0x2000, true);
        ui_memmap_layer(&ui.memmap, "Sound");
            ui_memmap_region(&ui.memmap, "ROM", 0x0000, 0x2000, true);
            ui_memmap_region(&ui.memmap, "RAM", 0x4000, 0x0400, true);
        ui_memmap_layer(&ui.memmap, "Chars");
            ui_memmap_region(&ui.memmap, "ROM 0", 0x0000, 0x1000, true);
            ui_memmap_region(&ui.memmap, "ROM 1", 0x1000, 0x1000, true);
            ui_memmap_region(&ui.memmap, "ROM 2", 0x2000, 0x1000, true);
        ui_memmap_layer(&ui.memmap, "Tiles");
            ui_memmap_region(&ui.memmap, "ROM 0", 0x0000, 0x2000, true);
            ui_memmap_region(&ui.memmap, "ROM 1", 0x2000, 0x2000, true);
            ui_memmap_region(&ui.memmap, "ROM 2", 0x4000, 0x2000, true);
        ui_memmap_layer(&ui.memmap, "Sprites");
            ui_memmap_region(&ui.memmap, "ROM 0", 0x0000, 0x2000, true);
            ui_memmap_region(&ui.memmap, "ROM 1", 0x2000, 0x2000, true);
            ui_memmap_region(&ui.memmap, "ROM 2", 0x4000, 0x2000, true);
        ui_memmap_layer(&ui.memmap, "Maps");
            ui_memmap_region(&ui.memmap, "ROM 0", 0x0000, 0x1000, true);
    }
    x += dx; y += dy;
    {
        ui_memedit_desc_t desc = {0};
        for (int i = 0; i < NUM_MEMLAYERS; i++) {
            desc.layers[i] = memlayer_names[i];
        }
        desc.read_cb = _ui_bombjack_mem_read;
        desc.write_cb = _ui_bombjack_mem_write;
        static const char* titles[] = { "Memory Editor #1", "Memory Editor #2", "Memory Editor #3", "Memory Editor #4" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_memedit_init(&ui.memedit[i], &desc);
            x += dx; y += dy;
        }
    }
    x += dx; y += dy;
    {
        ui_dasm_desc_t desc = {0};
        for (int i = 0; i < 2; i++) {
            desc.layers[i] = memlayer_names[i];
        }
        desc.cpu_type = UI_DASM_CPUTYPE_Z80;
        desc.start_addr = 0x0000;
        desc.read_cb = _ui_bombjack_mem_read;
        static const char* titles[4] = { "Disassembler #1", "Disassembler #2", "Disassembler #2", "Dissassembler #3" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_dasm_init(&ui.dasm[i], &desc);
            x += dx; y += dy;
        }
    }
}

void bombjackui_discard(void) {
    ui_dbg_discard(&ui.main.dbg);
    ui_dbg_discard(&ui.sound.dbg);
    ui_memmap_discard(&ui.memmap);
    ui_audio_discard(&ui.sound.audio);
    for (int i = 0; i < 3; i++) {
        ui_ay38910_discard(&ui.sound.psg[i]);
    }
    for (int i = 0; i < 4; i++) {
        ui_memedit_discard(&ui.memedit[i]);
        ui_dasm_discard(&ui.dasm[i]);
    }
    ui_z80_discard(&ui.main.cpu);
    ui_z80_discard(&ui.sound.cpu);
    ui_discard();
}

void bombjackui_exec(uint32_t frame_time_us) {
    const uint32_t slice_us = frame_time_us / 2;
    for (int i = 0; i < 2; i++) {
        /* tick the main board */
        if (ui_dbg_before_exec(&ui.main.dbg)) {
            uint64_t start = stm_now();
            uint32_t ticks_to_run = clk_ticks_to_run(&ui.bj->mainboard.clk, slice_us);
            uint32_t ticks_executed = z80_exec(&ui.bj->mainboard.cpu, ticks_to_run);
            clk_ticks_executed(&ui.bj->mainboard.clk, ticks_executed);
            ui.exec_time = stm_ms(stm_since(start));
            ui_dbg_after_exec(&ui.main.dbg);
        }
        /* tick the sound board */
        if (ui_dbg_before_exec(&ui.sound.dbg)) {
            uint32_t ticks_to_run = clk_ticks_to_run(&ui.bj->soundboard.clk, slice_us);
            uint32_t ticks_executed = z80_exec(&ui.bj->soundboard.cpu, ticks_to_run);
            clk_ticks_executed(&ui.bj->soundboard.clk, ticks_executed);
            ui_dbg_after_exec(&ui.sound.dbg);
        }
    }
    bombjack_decode_video(ui.bj);
}

} // extern "C"
