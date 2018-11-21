/*
    zx.c
    ZX Spectrum 48/128 emulator.
    - wait states when accessing contended memory are not emulated
    - video decoding works with scanline accuracy, not cycle accuracy
    - no tape or disc emulation
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/beeper.h"
#include "chips/ay38910.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/zx.h"
#include "zx-roms.h"
#ifdef CHIPS_USE_UI
#undef CHIPS_IMPL
#include "ui.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_z80.h"
#include "ui/ui_ay38910.h"
#endif

zx_t zx;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

#ifdef CHIPS_USE_UI
void zxui_init(void);
void zxui_discard(void);
void zxui_draw(void);
#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    fs_init();
    if (sargs_exists("file")) {
        fs_load_file(sargs_value("file"));
    }
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * ZX_DISPLAY_WIDTH,
        .height = 2 * ZX_DISPLAY_HEIGHT,
        .window_title = "ZX Spectrum",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* get zx_desc_t struct for given ZX type and joystick type */
zx_desc_t zx_desc(zx_type_t type, zx_joystick_type_t joy_type) {
    return (zx_desc_t){
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .rom_zx48k = dump_amstrad_zx48k,
        .rom_zx48k_size = sizeof(dump_amstrad_zx48k),
        .rom_zx128_0 = dump_amstrad_zx128k_0,
        .rom_zx128_0_size = sizeof(dump_amstrad_zx128k_0),
        .rom_zx128_1 = dump_amstrad_zx128k_1,
        .rom_zx128_1_size = sizeof(dump_amstrad_zx128k_1)
    };
}

/* one-time application init */
void app_init() {
    gfx_init(&(gfx_desc_t){
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        .top_offset = 16,
        .fb_width = ZX_DISPLAY_WIDTH,
        .fb_height = ZX_DISPLAY_HEIGHT + 16,
        #else
        .fb_width = ZX_DISPLAY_WIDTH,
        .fb_height = ZX_DISPLAY_HEIGHT,
        #endif
    });
    #ifdef CHIPS_USE_UI
    zxui_init();
    #endif
    clock_init();
    saudio_setup(&(saudio_desc){0});
    zx_type_t type = ZX_TYPE_128;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "zx48k")) {
            type = ZX_TYPE_48K;
        }
    }
    zx_joystick_type_t joy_type = ZX_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        if (sargs_equals("joystick", "kempston")) {
            joy_type = ZX_JOYSTICKTYPE_KEMPSTON;
        }
        else if (sargs_equals("joystick", "sinclair1")) {
            joy_type = ZX_JOYSTICKTYPE_SINCLAIR_1;
        }
        else if (sargs_equals("joystick", "sinclair2")) {
            joy_type = ZX_JOYSTICKTYPE_SINCLAIR_2;
        }
    }
    zx_desc_t desc = zx_desc(type, joy_type);
    zx_init(&zx, &desc);
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    #if CHIPS_USE_UI
        uint64_t start = stm_now();
        zx_exec(&zx, clock_frame_time());
        ui_set_exec_time(stm_since(start));
    #else
        zx_exec(&zx, clock_frame_time());
    #endif
    gfx_draw();
    if (fs_ptr() && clock_frame_count() > 120) {
        zx_quickload(&zx, fs_ptr(), fs_size());
        fs_free();
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        /* input was handled by UI */
        return;
    }
    #endif
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                zx_key_down(&zx, c);
                zx_key_up(&zx, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = 0x20; break;
                case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                case SAPP_KEYCODE_UP:           c = 0x0B; break;
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_BACKSPACE:    c = 0x0C; break;
                case SAPP_KEYCODE_ESCAPE:       c = 0x07; break;
                case SAPP_KEYCODE_LEFT_CONTROL: c = 0x0F; break; /* SymShift */
                default:                        c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    zx_key_down(&zx, c);
                }
                else {
                    zx_key_up(&zx, c);
                }
            }
            break;
        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
            sapp_show_keyboard(true);
            break;
        default:
            break;
    }
}

/* application cleanup callback */
void app_cleanup() {
    zx_discard(&zx);
    #ifdef CHIPS_USE_UI
    zxui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

/*=== optional debugging UI ==================================================*/
#ifdef CHIPS_USE_UI

static ui_memedit_t ui_memedit;
static ui_memmap_t ui_memmap;
static ui_dasm_t ui_dasm;
static ui_z80_t ui_cpu;
static ui_ay38910_t ui_ay;

/* menu handler functions */
void zxui_reset(void) { zx_reset(&zx); }
void zxui_boot_zx48k(void) { zx_desc_t desc = zx_desc(ZX_TYPE_48K, zx.joystick_type); zx_init(&zx, &desc); }
void zxui_boot_zx128(void) { zx_desc_t desc = zx_desc(ZX_TYPE_128, zx.joystick_type); zx_init(&zx, &desc); }

uint8_t* zxui_ptr(int layer, uint16_t addr) {
    if (0 == layer) {
        /* ZX128 ROM, RAM 5, RAM 2, RAM 0 */
        if (addr < 0x4000) {
            return &zx.rom[0][addr];
        }
        else if (addr < 0x8000) {
            return &zx.ram[5][addr - 0x4000];
        }
        else if (addr < 0xC000) {
            return &zx.ram[2][addr - 0x8000];
        }
        else {
            return &zx.ram[0][addr - 0xC000];
        }
    }
    else if (1 == layer) {
        /* 48K ROM, RAM 1 */
        if (addr < 0x4000) {
            return &zx.rom[1][addr];
        }
        else if (addr >= 0xC000) {
            return &zx.ram[1][addr - 0xC000];
        }
    }
    else if (layer < 8) {
        if (addr >= 0xC000) {
            return &zx.ram[layer][addr - 0xC000];
        }
    }
    /* fallthrough: unmapped memory */
    return 0;
}

uint8_t zxui_mem_read(int layer, uint16_t addr, void* user_data) {
    if ((layer == 0) || (ZX_TYPE_48K == zx.type)) {
        /* CPU visible layer */
        return mem_rd(&zx.mem, addr);
    }
    else {
        uint8_t* ptr = zxui_ptr(layer-1, addr);
        if (ptr) {
            return *ptr;
        }
        else {
            return 0xFF;
        }
    }
}

void zxui_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    if ((layer == 0) || (ZX_TYPE_48K == zx.type)) {
        mem_wr(&zx.mem, addr, data);
    }
    else {
        uint8_t* ptr = zxui_ptr(layer-1, addr);
        if (ptr) {
            *ptr = data;
        }
    }
}

void zxui_dummy(void) { }

void zxui_init(void) {
    ui_init(&(ui_desc_t) {
        .draw = zxui_draw,
        .menus = {
            {
                .name = "System",
                .items = {
                    { .name = "Reset", .func = zxui_reset },
                    { .name = "Boot to ZX Spectrum 48k", .func = zxui_boot_zx48k },
                    { .name = "Boot to ZX Spectrum 128", .func = zxui_boot_zx128 }
                }
            },
            {
                .name = "Hardware",
                .items = {
                    { .name = "Memory Map", .open = &ui_memmap.open },
                    { .name = "System State (TODO)", .func = zxui_dummy },
                    { .name = "Z80 CPU", .open = &ui_cpu.open },
                    { .name = "AY-3-8912", .open = &ui_ay.open },
                }
            },
            {
                .name = "Debug",
                .items = {
                    { .name = "Memory Editor", .open = &ui_memedit.open },
                    { .name = "Disassembler", .open = &ui_dasm.open },
                    { .name = "CPU Debugger (TODO)", .func = zxui_dummy },
                }
            }
        },
    });
    ui_memedit_init(&ui_memedit, &(ui_memedit_desc_t){
        .title = "Memory Editor",
        .layers = { "CPU Mapped", "Layer 0", "Layer 1", "Layer 2", "Layer 3", "Layer 4", "Layer 5", "Layer 6", "Layer 7" },
        .read_cb = zxui_mem_read,
        .write_cb = zxui_mem_write,
        .x = 20, .y = 40, .h = 120
    });
    ui_memmap_init(&ui_memmap, &(ui_memmap_desc_t){
        .title = "Memory Map",
        .x = 30, .y = 50, .w = 400, .h = 64
    });
    ui_dasm_init(&ui_dasm, &(ui_dasm_desc_t){
        .title = "Disassembler",
        .layers = { "CPU Mapped", "Layer 0", "Layer 1", "Layer 2", "Layer 3", "Layer 4", "Layer 5", "Layer 6", "Layer 7" },
        .start_addr = 0x0000,
        .read_cb = zxui_mem_read,
        .x = 40, .y = 60, .w = 400, .h = 256
    });
    ui_z80_init(&ui_cpu, &(ui_z80_desc_t){
        .title = "Z80 CPU",
        .cpu = &zx.cpu,
        .x = 40, .y = 60,
        .chip_desc = {
            .name = "Z80\nCPU",
            .num_slots = 36,
            .pins = {
                { .name = "D0",      .slot = 0, .mask = Z80_D0 },
                { .name = "D1",      .slot = 1, .mask = Z80_D1 },
                { .name = "D2",      .slot = 2, .mask = Z80_D2 },
                { .name = "D3",      .slot = 3, .mask = Z80_D3 },
                { .name = "D4",      .slot = 4, .mask = Z80_D4 },
                { .name = "D5",      .slot = 5, .mask = Z80_D5 },
                { .name = "D6",      .slot = 6, .mask = Z80_D6 },
                { .name = "D7",      .slot = 7, .mask = Z80_D7 },
                { .name = "M1",      .slot = 9, .mask = Z80_M1 },
                { .name = "MREQ",    .slot = 10, .mask = Z80_MREQ },
                { .name = "IORQ",    .slot = 11, .mask = Z80_IORQ },
                { .name = "RD",      .slot = 12, .mask = Z80_RD },
                { .name = "WR",      .slot = 13, .mask = Z80_WR },
                { .name = "HALT",    .slot = 14, .mask = Z80_HALT },
                { .name = "INT",     .slot = 15, .mask = Z80_INT },
                { .name = "NMI",     .slot = 16, .mask = Z80_NMI },
                { .name = "WAIT",    .slot = 17, .mask = Z80_WAIT_MASK },
                { .name = "A0",      .slot = 18, .mask = Z80_A0 },
                { .name = "A1",      .slot = 19, .mask = Z80_A1 },
                { .name = "A2",      .slot = 20, .mask = Z80_A2 },
                { .name = "A3",      .slot = 21, .mask = Z80_A3 },
                { .name = "A4",      .slot = 22, .mask = Z80_A4 },
                { .name = "A5",      .slot = 23, .mask = Z80_A5 },
                { .name = "A6",      .slot = 24, .mask = Z80_A6 },
                { .name = "A7",      .slot = 25, .mask = Z80_A7 },
                { .name = "A8",      .slot = 26, .mask = Z80_A8 },
                { .name = "A9",      .slot = 27, .mask = Z80_A9 },
                { .name = "A10",     .slot = 28, .mask = Z80_A10 },
                { .name = "A11",     .slot = 29, .mask = Z80_A11 },
                { .name = "A12",     .slot = 30, .mask = Z80_A12 },
                { .name = "A13",     .slot = 31, .mask = Z80_A13 },
                { .name = "A14",     .slot = 32, .mask = Z80_A14 },
                { .name = "A15",     .slot = 33, .mask = Z80_A15 }
            }
        }
    });
    ui_ay38910_init(&ui_ay, &(ui_ay38910_desc_t){
        .title = "AY-3-8912",
        .ay = &zx.ay,
        .x = 40, .y = 60,
        .chip_desc = {
            .name = "8912",
            .num_slots = 22,
            .pins = {
                { .name = "DA0",  .slot = 0, .mask = AY38910_DA0 },
                { .name = "DA1",  .slot = 1, .mask = AY38910_DA1 },
                { .name = "DA2",  .slot = 2, .mask = AY38910_DA2 },
                { .name = "DA3",  .slot = 3, .mask = AY38910_DA3 },
                { .name = "DA4",  .slot = 4, .mask = AY38910_DA4 },
                { .name = "DA5",  .slot = 5, .mask = AY38910_DA5 },
                { .name = "DA6",  .slot = 6, .mask = AY38910_DA6 },
                { .name = "DA7",  .slot = 7, .mask = AY38910_DA7 },
                { .name = "BDIR", .slot = 9, .mask = AY38910_BDIR },
                { .name = "BC1",  .slot = 10, .mask = AY38910_BC1 },
                { .name = "IOA0", .slot = 11, .mask = AY38910_IOA0 },
                { .name = "IOA1", .slot = 12, .mask = AY38910_IOA1 },
                { .name = "IOA2", .slot = 13, .mask = AY38910_IOA2 },
                { .name = "IOA3", .slot = 14, .mask = AY38910_IOA3 },
                { .name = "IOA4", .slot = 15, .mask = AY38910_IOA4 },
                { .name = "IOA5", .slot = 16, .mask = AY38910_IOA5 },
                { .name = "IOA6", .slot = 17, .mask = AY38910_IOA6 },
                { .name = "IOA7", .slot = 18, .mask = AY38910_IOA7 }
            }
        }
    });
}

void zxui_discard(void) {
    ui_z80_discard(&ui_cpu);
    ui_ay38910_discard(&ui_ay);
    ui_dasm_discard(&ui_dasm);
    ui_memmap_discard(&ui_memmap);
    ui_memedit_discard(&ui_memedit);
}

void zxui_update_memmap(void) {
    ui_memmap_reset(&ui_memmap);
    if (ZX_TYPE_48K == zx.type) {
        ui_memmap_layer(&ui_memmap, "System");
            ui_memmap_region(&ui_memmap, "ROM", 0x0000, 0x4000, true);
            ui_memmap_region(&ui_memmap, "RAM", 0x4000, 0xC000, true);
    }
    else {
        const uint8_t m = zx.last_mem_config;
        ui_memmap_layer(&ui_memmap, "Layer 0");
            ui_memmap_region(&ui_memmap, "ZX128 ROM", 0x0000, 0x4000, !(m & (1<<4)));
            ui_memmap_region(&ui_memmap, "RAM 5", 0x4000, 0x4000, true);
            ui_memmap_region(&ui_memmap, "RAM 2", 0x8000, 0x4000, true);
            ui_memmap_region(&ui_memmap, "RAM 0", 0xC000, 0x4000, 0 == (m & 7));
        ui_memmap_layer(&ui_memmap, "Layer 1");
            ui_memmap_region(&ui_memmap, "ZX48K ROM", 0x0000, 0x4000, m & (1<<4));
            ui_memmap_region(&ui_memmap, "RAM 1", 0xC000, 0x4000, 1 == (m & 7));
        ui_memmap_layer(&ui_memmap, "Layer 2");
            ui_memmap_region(&ui_memmap, "RAM 2", 0xC000, 0x4000, 2 == (m & 7));
        ui_memmap_layer(&ui_memmap, "Layer 3");
            ui_memmap_region(&ui_memmap, "RAM 3", 0xC000, 0x4000, 3 == (m & 7));
        ui_memmap_layer(&ui_memmap, "Layer 4");
            ui_memmap_region(&ui_memmap, "RAM 4", 0xC000, 0x4000, 4 == (m & 7));
        ui_memmap_layer(&ui_memmap, "Layer 5");
            ui_memmap_region(&ui_memmap, "RAM 5", 0xC000, 0x4000, 5 == (m & 7));
        ui_memmap_layer(&ui_memmap, "Layer 6");
            ui_memmap_region(&ui_memmap, "RAM 6", 0xC000, 0x4000, 6 == (m & 7));
        ui_memmap_layer(&ui_memmap, "Layer 7");
            ui_memmap_region(&ui_memmap, "RAM 7", 0xC000, 0x4000, 7 == (m & 7));
    }
}

void zxui_draw() {
    if (ui_memmap.open) {
        zxui_update_memmap();
    }
    ui_ay38190_draw(&ui_ay);
    ui_z80_draw(&ui_cpu);
    ui_memedit_draw(&ui_memedit);
    ui_memmap_draw(&ui_memmap);
    ui_dasm_draw(&ui_dasm);
}
#endif /* CHIPS_USE_UI */
