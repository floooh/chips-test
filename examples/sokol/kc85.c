/*
    kc85.c

    KC85/2, /3 and /4.
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/kc85.h"
#include "kc85-roms.h"
#ifdef CHIPS_USE_UI
#undef CHIPS_IMPL
#include "ui.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_z80.h"
#include "ui/ui_z80pio.h"
#include "ui/ui_z80ctc.h"
#include "ui/ui_kc85.h"
#endif

kc85_t kc85;

/* module to insert after ROM module image has been loaded */
kc85_module_type_t delay_insert_module = KC85_MODULE_NONE;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

#ifdef CHIPS_USE_UI
void kc85ui_init(void);
void kc85ui_discard(void);
void kc85ui_draw(void);
#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc) { .argc = argc, .argv = argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * KC85_DISPLAY_WIDTH,
        .height = 2 * KC85_DISPLAY_HEIGHT,
        .window_title = "KC85",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* a callback to patch some known problems in game snapshot files */
static void patch_snapshots(const char* snapshot_name, void* user_data) {
    if (strcmp(snapshot_name, "JUNGLE     ") == 0) {
        /* patch start level 1 into memory */
        mem_wr(&kc85.mem, 0x36b7, 1);
        mem_wr(&kc85.mem, 0x3697, 1);
        for (int i = 0; i < 5; i++) {
            mem_wr(&kc85.mem, 0x1770 + i, mem_rd(&kc85.mem, 0x36b6 + i));
        }
    }
    else if (strcmp(snapshot_name, "DIGGER  COM\x01") == 0) {
        mem_wr16(&kc85.mem, 0x09AA, 0x0160);    /* time for delay-loop 0160 instead of 0260 */
        mem_wr(&kc85.mem, 0x3d3a, 0xB5);        /* OR L instead of OR (HL) */
    }
    else if (strcmp(snapshot_name, "DIGGERJ") == 0) {
        mem_wr16(&kc85.mem, 0x09AA, 0x0260);
        mem_wr(&kc85.mem, 0x3d3a, 0xB5);       /* OR L instead of OR (HL) */
    }
}

/* get kc85_desc_t struct for a given KC85 type */
static kc85_desc_t kc85_desc(kc85_type_t type) {
    return (kc85_desc_t) {
        .type = type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .patch_cb = patch_snapshots,
        .rom_caos22 = dump_caos22,
        .rom_caos22_size = sizeof(dump_caos22),
        .rom_caos31 = dump_caos31,
        .rom_caos31_size = sizeof(dump_caos31),
        .rom_caos42c = dump_caos42c,
        .rom_caos42c_size = sizeof(dump_caos42c),
        .rom_caos42e = dump_caos42e,
        .rom_caos42e_size = sizeof(dump_caos42e),
        .rom_kcbasic = dump_basic_c0,
        .rom_kcbasic_size = sizeof(dump_basic_c0)
    };
}

void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        .top_offset = 16,
        .fb_width = KC85_DISPLAY_WIDTH,
        .fb_height = KC85_DISPLAY_HEIGHT + 16,
        #else
        .fb_width = KC85_DISPLAY_WIDTH,
        .fb_height = KC85_DISPLAY_HEIGHT,
        #endif
    });
    #ifdef CHIPS_USE_UI
    kc85ui_init();
    #endif
    keybuf_init(6);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    /* specific KC85 model? */
    kc85_type_t type = KC85_TYPE_2;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "kc85_3")) {
            type = KC85_TYPE_3;
        }
        else if (sargs_equals("type", "kc85_4")) {
            type = KC85_TYPE_4;
        }
    }
    /* initialize the KC85 emulator */
    kc85_desc_t desc = kc85_desc(type);
    kc85_init(&kc85, &desc);
    bool delay_input = false;
    /* snapshot file or rom-module image */
    if (sargs_exists("snapshot")) {
        delay_input=true;
        fs_load_file(sargs_value("snapshot"));
    }
    else if (sargs_exists("mod_image")) {
        fs_load_file(sargs_value("mod_image"));
    }
    /* check if any modules should be inserted */
    if (sargs_exists("mod")) {
        /* RAM modules can be inserted immediately, ROM modules
           only after the ROM image has been loaded
        */
        if (sargs_equals("mod", "m022")) {
            kc85_insert_ram_module(&kc85, 0x08, KC85_MODULE_M022_16KBYTE);
        }
        else if (sargs_equals("mod", "m011")) {
            kc85_insert_ram_module(&kc85, 0x08, KC85_MODULE_M011_64KBYE);
        }
        else {
            /* a ROM module */
            delay_input = true;
            if (sargs_equals("mod", "m006")) {
                delay_insert_module = KC85_MODULE_M006_BASIC;
            }
            else if (sargs_equals("mod", "m012")) {
                delay_insert_module = KC85_MODULE_M012_TEXOR;
            }
            else if (sargs_equals("mod", "m026")) {
                delay_insert_module = KC85_MODULE_M026_FORTH;
            }
            else if (sargs_equals("mod", "m027")) {
                delay_insert_module = KC85_MODULE_M027_DEVELOPMENT;
            }
        }
    }
    /* keyboard input to send to emulator */
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

void app_frame(void) {
    #if CHIPS_USE_UI
        uint64_t start = stm_now();
        kc85_exec(&kc85, clock_frame_time());
        ui_set_exec_time(stm_since(start));
    #else
        kc85_exec(&kc85, clock_frame_time());
    #endif
    gfx_draw();
    uint32_t delay_frames = kc85.type == KC85_TYPE_4 ? 180 : 480;
    if (fs_ptr() && clock_frame_count() > delay_frames) {
        if (sargs_exists("snapshot")) {
            kc85_quickload(&kc85, fs_ptr(), fs_size());
            if (sargs_exists("input")) {
                keybuf_put(sargs_value("input"));
            }
        }
        else if (sargs_exists("mod_image")) {
            /* insert the rom module */
            if (delay_insert_module != KC85_MODULE_NONE) {
                kc85_insert_rom_module(&kc85, 0x08, delay_insert_module, fs_ptr(), fs_size());
            }
            if (sargs_exists("input")) {
                keybuf_put(sargs_value("input"));
            }
        }
        fs_free();
    }
    uint8_t key_code;
    if (0 != (key_code = keybuf_get())) {
        kc85_key_down(&kc85, key_code);
        kc85_key_up(&kc85, key_code);
    }
}

void app_input(const sapp_event* event) {
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        /* input was handled by UI */
        return;
    }
    #endif
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                /* need to invert case (unshifted is upper caps, shifted is lower caps */
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                kc85_key_down(&kc85, c);
                kc85_key_up(&kc85, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = shift ? 0x5B : 0x20; break; /* 0x5B: neg space, 0x20: space */
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                case SAPP_KEYCODE_UP:           c = 0x0B; break;
                case SAPP_KEYCODE_HOME:         c = 0x10; break;
                case SAPP_KEYCODE_INSERT:       c = shift ? 0x0C : 0x1A; break; /* 0x0C: cls, 0x1A: ins */
                case SAPP_KEYCODE_BACKSPACE:    c = shift ? 0x0C : 0x01; break; /* 0x0C: cls, 0x01: del */ 
                case SAPP_KEYCODE_ESCAPE:       c = shift ? 0x13 : 0x03; break; /* 0x13: stop, 0x03: brk */
                case SAPP_KEYCODE_F1:           c = 0xF1; break;
                case SAPP_KEYCODE_F2:           c = 0xF2; break;
                case SAPP_KEYCODE_F3:           c = 0xF3; break;
                case SAPP_KEYCODE_F4:           c = 0xF4; break;
                case SAPP_KEYCODE_F5:           c = 0xF5; break;
                case SAPP_KEYCODE_F6:           c = 0xF6; break;
                case SAPP_KEYCODE_F7:           c = 0xF7; break;
                case SAPP_KEYCODE_F8:           c = 0xF8; break;
                case SAPP_KEYCODE_F9:           c = 0xF9; break;
                case SAPP_KEYCODE_F10:          c = 0xFA; break;
                case SAPP_KEYCODE_F11:          c = 0xFB; break;
                case SAPP_KEYCODE_F12:          c = 0xFC; break;
                default:                        c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    kc85_key_down(&kc85, c);
                }
                else {
                    kc85_key_up(&kc85, c);
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

void app_cleanup(void) {
    kc85_discard(&kc85);
    #ifdef CHIPS_USE_UI
    kc85ui_discard();
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
static ui_z80pio_t ui_pio;
static ui_z80ctc_t ui_ctc;
static ui_kc85_t ui_kc85;

/* menu handler functions */
void kc85ui_reset(void) { kc85_reset(&kc85); }
void kc85ui_boot_kc852(void) { kc85_desc_t desc = kc85_desc(KC85_TYPE_2); kc85_init(&kc85, &desc); }
void kc85ui_boot_kc853(void) { kc85_desc_t desc = kc85_desc(KC85_TYPE_3); kc85_init(&kc85, &desc); }
void kc85ui_boot_kc854(void) { kc85_desc_t desc = kc85_desc(KC85_TYPE_4); kc85_init(&kc85, &desc); }

uint8_t kc85ui_mem_read(int layer, uint16_t addr, void* user_data) {
    if (layer == 0) {
        return mem_rd(&kc85.mem, addr);
    }
    else {
        return mem_layer_rd(&kc85.mem, layer-1, addr);
    }
}

void kc85ui_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    if (layer == 0) {
        mem_wr(&kc85.mem, addr, data);
    }
    else {
        mem_layer_wr(&kc85.mem, layer-1, addr, data);
    }
}

void kc85ui_dummy(void) { }

void kc85ui_init(void) {
    ui_init(&(ui_desc_t) {
        .draw = kc85ui_draw,
        .menus = {
            {
                .name = "System",
                .items = {
                    { .name = "Reset", .func = kc85ui_reset },
                    { .name = "Boot to KC85/2", .func = kc85ui_boot_kc852 },
                    { .name = "Boot to KC85/3", .func = kc85ui_boot_kc853 },
                    { .name = "Boot to KC85/4", .func = kc85ui_boot_kc854 }
                }
            },
            {
                .name = "Hardware",
                .items = {
                    { .name = "Memory Map", .open = &ui_memmap.open },
                    { .name = "System State", .open = &ui_kc85.open },
                    { .name = "Z80 CPU", .open = &ui_cpu.open },
                    { .name = "Z80 PIO", .open = &ui_pio.open },
                    { .name = "Z80 CTC", .open = &ui_ctc.open },
                }
            },
            {
                .name = "Debug",
                .items = {
                    { .name = "Memory Editor", .open = &ui_memedit.open },
                    { .name = "Disassembler", .open = &ui_dasm.open },
                    { .name = "CPU Debugger (TODO)", .func = kc85ui_dummy },
                    { .name = "Scan Commands (TODO)", .func = kc85ui_dummy }
                }
            }
        },
    });
    ui_memedit_init(&ui_memedit, &(ui_memedit_desc_t){
        .title = "Memory Editor",
        .layers = { "CPU Mapped", "Motherboard", "Slot 08", "Slot 0C" },
        .read_cb = kc85ui_mem_read,
        .write_cb = kc85ui_mem_write,
        .x = 20, .y = 40, .h = 120
    });
    ui_memmap_init(&ui_memmap, &(ui_memmap_desc_t){
        .title = "Memory Map",
        .x = 30, .y = 50, .w = 400, .h = 64
    });
    ui_dasm_init(&ui_dasm, &(ui_dasm_desc_t){
        .title = "Disassembler",
        .layers = { "CPU Mapped", "Motherboard", "Slot 08", "Slot 0C" },
        .start_addr = 0xF000,
        .read_cb = kc85ui_mem_read,
        .x = 40, .y = 60, .w = 400, .h = 256
    });
    ui_kc85_init(&ui_kc85, &(ui_kc85_desc_t){
        .title = "System State",
        .kc85 = &kc85,
        .x = 40, .y = 60
    });
    ui_z80pio_init(&ui_pio, &(ui_z80pio_desc_t){
        .title = "Z80 PIO",
        .pio = &kc85.pio,
        .x = 40, .y = 60,
        .chip_desc = {
            .name = "Z80\nPIO",
            .num_slots = 40,
            .pins = {
                { .name = "D0",      .slot = 0, .mask = Z80_D0 },
                { .name = "D1",      .slot = 1, .mask = Z80_D1 },
                { .name = "D2",      .slot = 2, .mask = Z80_D2 },
                { .name = "D3",      .slot = 3, .mask = Z80_D3 },
                { .name = "D4",      .slot = 4, .mask = Z80_D4 },
                { .name = "D5",      .slot = 5, .mask = Z80_D5 },
                { .name = "D6",      .slot = 6, .mask = Z80_D6 },
                { .name = "D7",      .slot = 7, .mask = Z80_D7 },
                { .name = "CE",      .slot = 9, .mask = Z80PIO_CE },
                { .name = "BASEL",   .slot = 10, .mask = Z80PIO_BASEL },
                { .name = "CDSEL",   .slot = 11, .mask = Z80PIO_CDSEL },
                { .name = "M1",      .slot = 12, .mask = Z80PIO_M1 },
                { .name = "IORQ",    .slot = 13, .mask = Z80PIO_IORQ },
                { .name = "RD",      .slot = 14, .mask = Z80PIO_RD },
                { .name = "INT",     .slot = 15, .mask = Z80PIO_INT },
                { .name = "ARDY",    .slot = 20, .mask = Z80PIO_ARDY },
                { .name = "ASTB",    .slot = 21, .mask = Z80PIO_ASTB },
                { .name = "PA0",     .slot = 22, .mask = Z80PIO_PA0 },
                { .name = "PA1",     .slot = 23, .mask = Z80PIO_PA1 },
                { .name = "PA2",     .slot = 24, .mask = Z80PIO_PA2 },
                { .name = "PA3",     .slot = 25, .mask = Z80PIO_PA3 },
                { .name = "PA4",     .slot = 26, .mask = Z80PIO_PA4 },
                { .name = "PA5",     .slot = 27, .mask = Z80PIO_PA5 },
                { .name = "PA6",     .slot = 28, .mask = Z80PIO_PA6 },
                { .name = "PA7",     .slot = 29, .mask = Z80PIO_PA7 },
                { .name = "BRDY",    .slot = 30, .mask = Z80PIO_ARDY },
                { .name = "BSTB",    .slot = 31, .mask = Z80PIO_ASTB },
                { .name = "PB0",     .slot = 32, .mask = Z80PIO_PB0 },
                { .name = "PB1",     .slot = 33, .mask = Z80PIO_PB1 },
                { .name = "PB2",     .slot = 34, .mask = Z80PIO_PB2 },
                { .name = "PB3",     .slot = 35, .mask = Z80PIO_PB3 },
                { .name = "PB4",     .slot = 36, .mask = Z80PIO_PB4 },
                { .name = "PB5",     .slot = 37, .mask = Z80PIO_PB5 },
                { .name = "PB6",     .slot = 38, .mask = Z80PIO_PB6 },
                { .name = "PB7",     .slot = 39, .mask = Z80PIO_PB7 },
            }
        }
    });
    ui_z80ctc_init(&ui_ctc, &(ui_z80ctc_desc_t){
        .title = "Z80 CTC",
        .ctc = &kc85.ctc,
        .x = 40, .y = 60,
        .chip_desc = {
            .name = "Z80\nCTC",
            .num_slots = 32,
            .pins = {
                { .name = "D0",     .slot = 0, .mask = Z80_D0 },
                { .name = "D1",     .slot = 1, .mask = Z80_D1 },
                { .name = "D2",     .slot = 2, .mask = Z80_D2 },
                { .name = "D3",     .slot = 3, .mask = Z80_D3 },
                { .name = "D4",     .slot = 4, .mask = Z80_D4 },
                { .name = "D5",     .slot = 5, .mask = Z80_D5 },
                { .name = "D6",     .slot = 6, .mask = Z80_D6 },
                { .name = "D7",     .slot = 7, .mask = Z80_D7 },
                { .name = "CE",     .slot = 9, .mask = Z80CTC_CE },
                { .name = "CS0",    .slot = 10, .mask = Z80CTC_CS0 },
                { .name = "CS1",    .slot = 11, .mask = Z80CTC_CS1 },
                { .name = "M1",     .slot = 12, .mask = Z80CTC_M1 },
                { .name = "IORQ",   .slot = 13, .mask = Z80CTC_IORQ },
                { .name = "RD",     .slot = 14, .mask = Z80CTC_RD },
                { .name = "INT",    .slot = 15, .mask = Z80CTC_INT },
                { .name = "CT0",    .slot = 16, .mask = Z80CTC_CLKTRG0 },
                { .name = "ZT0",    .slot = 17, .mask = Z80CTC_ZCTO0 },
                { .name = "CT1",    .slot = 19, .mask = Z80CTC_CLKTRG1 },
                { .name = "ZT1",    .slot = 20, .mask = Z80CTC_ZCTO1 },
                { .name = "CT2",    .slot = 22, .mask = Z80CTC_CLKTRG2 },
                { .name = "ZT2",    .slot = 23, .mask = Z80CTC_ZCTO2 },
                { .name = "CT3",    .slot = 25, .mask = Z80CTC_CLKTRG3 }
            }
        }
    });
    ui_z80_init(&ui_cpu, &(ui_z80_desc_t){
        .title = "Z80 CPU",
        .cpu = &kc85.cpu,
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
}

void kc85ui_discard(void) {
    ui_kc85_discard(&ui_kc85);
    ui_z80ctc_discard(&ui_ctc);
    ui_z80pio_discard(&ui_pio);
    ui_z80_discard(&ui_cpu);
    ui_dasm_discard(&ui_dasm);
    ui_memmap_discard(&ui_memmap);
    ui_memedit_discard(&ui_memedit);
}

void kc85ui_update_memmap(void) {
    const uint8_t pio_a = kc85.pio_a;
    const uint8_t pio_b = kc85.pio_b;
    const uint8_t io86  = kc85.io86;
    const uint8_t io84  = kc85.io84;
    ui_memmap_reset(&ui_memmap);
    if (KC85_TYPE_2 == kc85.type) {
        /* KC85/2 memory map */
        ui_memmap_layer(&ui_memmap, "System");
            ui_memmap_region(&ui_memmap, "RAM0", 0x0000, 0x4000, pio_a & KC85_PIO_A_RAM);
            ui_memmap_region(&ui_memmap, "IRM", 0x8000, 0x4000, pio_a & KC85_PIO_A_IRM);
            ui_memmap_region(&ui_memmap, "CAOS ROM 1", 0xE000, 0x0800, pio_a & KC85_PIO_A_CAOS_ROM);
            ui_memmap_region(&ui_memmap, "CAOS ROM 2", 0xF000, 0x0800, pio_a & KC85_PIO_A_CAOS_ROM);
    }
    else if (KC85_TYPE_3 == kc85.type) {
        /* KC85/3 memory map */
        ui_memmap_layer(&ui_memmap, "System");
            ui_memmap_region(&ui_memmap, "RAM0", 0x0000, 0x4000, pio_a & KC85_PIO_A_RAM);
            ui_memmap_region(&ui_memmap, "IRM", 0x8000, 0x4000, pio_a & KC85_PIO_A_IRM);
            ui_memmap_region(&ui_memmap, "BASIC ROM", 0xC000, 0x2000, pio_a & KC85_PIO_A_BASIC_ROM);
            ui_memmap_region(&ui_memmap, "CAOS ROM", 0xE000, 0x2000, pio_a & KC85_PIO_A_CAOS_ROM);
    }
    else {
        /* KC85/4 memory map */
        ui_memmap_layer(&ui_memmap, "System 0");
            ui_memmap_region(&ui_memmap, "RAM0", 0x0000, 0x4000, pio_a & KC85_PIO_A_RAM);
            ui_memmap_region(&ui_memmap, "RAM4", 0x4000, 0x4000, io86 & KC85_IO86_RAM4);
            ui_memmap_region(&ui_memmap, "IRM0 PIXELS", 0x8000, 0x2800, (pio_a & KC85_PIO_A_IRM) && ((io84 & 6)==0));
            ui_memmap_region(&ui_memmap, "IRM0", 0xA800, 0x1800, pio_a & KC85_PIO_A_IRM);
            ui_memmap_region(&ui_memmap, "CAOS ROM E", 0xE000, 0x2000, pio_a & KC85_PIO_A_CAOS_ROM);
        ui_memmap_layer(&ui_memmap, "System 1");
            ui_memmap_region(&ui_memmap, "IRM0 COLORS", 0x8000, 0x2800, (pio_a & KC85_PIO_A_IRM) && ((io84 & 6)==2));
            ui_memmap_region(&ui_memmap, "CAOS ROM C", 0xC000, 0x1000, io86 & KC85_IO86_CAOS_ROM_C);
        ui_memmap_layer(&ui_memmap, "System 2");
            ui_memmap_region(&ui_memmap, "IRM1 PIXELS", 0x8000, 0x2800, (pio_a & KC85_PIO_A_IRM) && ((io84 & 6)==4));
            ui_memmap_region(&ui_memmap, "BASIC ROM", 0xC000, 0x2000, pio_a & KC85_PIO_A_BASIC_ROM);
        ui_memmap_layer(&ui_memmap, "System 3");
            ui_memmap_region(&ui_memmap, "IRM1 COLORS", 0x8000, 0x2800, (pio_a & KC85_PIO_A_IRM) && ((io84 & 6)==6));
        ui_memmap_layer(&ui_memmap, "System 4");
            ui_memmap_region(&ui_memmap, "RAM8 BANK0", 0x8000, 0x4000, (pio_b & KC85_PIO_B_RAM8) && !(io84 & KC85_IO84_SEL_RAM8));
        ui_memmap_layer(&ui_memmap, "System 5");
            ui_memmap_region(&ui_memmap, "RAM8 BANK1", 0x8000, 0x4000, (pio_b & KC85_PIO_B_RAM8) && (io84 & KC85_IO84_SEL_RAM8));
    }
    for (int i = 0; i < KC85_NUM_SLOTS; i++) {
        const uint8_t slot_addr = kc85.exp.slot[i].addr;
        ui_memmap_layer(&ui_memmap, slot_addr == 0x08 ? "Slot 08" : "Slot 0C");
        if (kc85_slot_occupied(&kc85, slot_addr)) {
            ui_memmap_region(&ui_memmap,
                kc85_slot_mod_name(&kc85, slot_addr),
                kc85_slot_cpu_addr(&kc85, slot_addr),
                kc85_slot_mod_size(&kc85, slot_addr),
                kc85_slot_ctrl(&kc85, slot_addr) & 1);
        }
    }
}

void kc85ui_draw() {
    if (ui_memmap.open) {
        kc85ui_update_memmap();
    }
    ui_z80_draw(&ui_cpu);
    ui_z80pio_draw(&ui_pio);
    ui_z80ctc_draw(&ui_ctc);
    ui_memedit_draw(&ui_memedit);
    ui_memmap_draw(&ui_memmap);
    ui_dasm_draw(&ui_dasm);
    ui_kc85_draw(&ui_kc85);
}
#endif /* CHIPS_USE_UI */
