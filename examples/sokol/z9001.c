//------------------------------------------------------------------------------
//  z9001.c
//
//  Wiring diagram: http://www.sax.de/~zander/kc/kcsch_1.pdf
//  Detailed Manual: http://www.sax.de/~zander/z9001/doku/z9_fub.pdf
//
//  not emulated: beeper sound, border color, 40x20 video mode
//------------------------------------------------------------------------------
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/z80ctc.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/z9001.h"
#include "z9001-roms.h"
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
#endif

z9001_t z9001;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

#ifdef CHIPS_USE_UI
void z9001ui_init(void);
void z9001ui_discard(void);
void z9001ui_draw(void);
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
        .width = 2 * Z9001_DISPLAY_WIDTH,
        .height = 2 * Z9001_DISPLAY_HEIGHT,
        .window_title = "Robotron Z9001/KC87",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* get a z9001_desc_t struct for given Z9001 model */
static z9001_desc_t z9001_desc(z9001_type_t type) {
    return (z9001_desc_t) {
        .type = type,
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_z9001_os_1 = dump_z9001_os12_1,
        .rom_z9001_os_1_size = sizeof(dump_z9001_os12_1),
        .rom_z9001_os_2 = dump_z9001_os12_2,
        .rom_z9001_os_2_size = sizeof(dump_z9001_os12_2),
        .rom_z9001_basic = dump_z9001_basic_507_511,
        .rom_z9001_basic_size = sizeof(dump_z9001_basic_507_511),
        .rom_z9001_font = dump_z9001_font,
        .rom_z9001_font_size = sizeof(dump_z9001_font),
        .rom_kc87_os = dump_kc87_os_2,
        .rom_kc87_os_size = sizeof(dump_kc87_os_2),
        .rom_kc87_basic = dump_z9001_basic,
        .rom_kc87_basic_size = sizeof(dump_z9001_basic),
        .rom_kc87_font = dump_kc87_font_2,
        .rom_kc87_font_size = sizeof(dump_kc87_font_2)
    };
}

/* one-time application init */
void app_init() {
    gfx_init(&(gfx_desc_t) {
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        .top_offset = 16,
        .fb_width = Z9001_DISPLAY_WIDTH,
        .fb_height = Z9001_DISPLAY_HEIGHT + 16,
        #else
        .fb_width = Z9001_DISPLAY_WIDTH,
        .fb_height = Z9001_DISPLAY_HEIGHT,
        #endif
    });
    #ifdef CHIPS_USE_UI
    z9001ui_init();
    #endif
    clock_init();
    saudio_setup(&(saudio_desc){0});
    z9001_type_t type = Z9001_TYPE_Z9001;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "kc87")) {
            type = Z9001_TYPE_KC87;
        }
    }
    z9001_desc_t desc = z9001_desc(type);
    z9001_init(&z9001, &desc);
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    #if CHIPS_USE_UI
        uint64_t start = stm_now();
        z9001_exec(&z9001, clock_frame_time());
        ui_set_exec_time(stm_since(start));
    #else
        z9001_exec(&z9001, clock_frame_time());
    #endif
    gfx_draw();
    if (fs_ptr() && clock_frame_count() > 20) {
        z9001_quickload(&z9001, fs_ptr(), fs_size());
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
            if ((c >= 0x20) && (c < 0x7F)) {
                /* need to invert case (unshifted is upper caps, shifted is lower caps */
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                z9001_key_down(&z9001, c);
                z9001_key_up(&z9001, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_ENTER:    c = 0x0D; break;
                case SAPP_KEYCODE_RIGHT:    c = 0x09; break;
                case SAPP_KEYCODE_LEFT:     c = 0x08; break;
                case SAPP_KEYCODE_DOWN:     c = 0x0A; break;
                case SAPP_KEYCODE_UP:       c = 0x0B; break;
                case SAPP_KEYCODE_ESCAPE:   c = (event->modifiers & SAPP_MODIFIER_SHIFT)? 0x1B: 0x03; break;
                case SAPP_KEYCODE_INSERT:   c = 0x1A; break;
                case SAPP_KEYCODE_HOME:     c = 0x19; break;
                default:                    c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    z9001_key_down(&z9001, c);
                }
                else {
                    z9001_key_up(&z9001, c);
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
    z9001_discard(&z9001);
    #ifdef CHIPS_USE_UI
    z9001ui_discard();
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
static ui_z80pio_t ui_pio_1;
static ui_z80pio_t ui_pio_2;
static ui_z80ctc_t ui_ctc;

/* menu handler functions */
void z9001ui_reset(void) { z9001_reset(&z9001); }
void z9001ui_boot_z9001(void) { z9001_desc_t desc = z9001_desc(Z9001_TYPE_Z9001); z9001_init(&z9001, &desc); }
void z9001ui_boot_kc87(void) { z9001_desc_t desc = z9001_desc(Z9001_TYPE_KC87); z9001_init(&z9001, &desc); }
void z9001ui_memedit_toggle(void) { ui_memedit_toggle(&ui_memedit); }
void z9001ui_memmap_toggle(void) { ui_memmap_toggle(&ui_memmap); }
void z9001ui_dasm_toggle(void) { ui_dasm_toggle(&ui_dasm); }
void z9001ui_cpu_toggle(void) { ui_z80_toggle(&ui_cpu); }
void z9001ui_pio1_toggle(void) { ui_z80pio_toggle(&ui_pio_1); }
void z9001ui_pio2_toggle(void) { ui_z80pio_toggle(&ui_pio_2); }
void z9001ui_ctc_toggle(void) { ui_z80ctc_toggle(&ui_ctc); }

uint8_t z9001ui_mem_read(int layer, uint16_t addr, void* user_data) {
    return mem_rd(&z9001.mem, addr);
}

void z9001ui_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    mem_wr(&z9001.mem, addr, data);
}

void z9001ui_dummy(void) { }

void z9001ui_init(void) {
    ui_init(&(ui_desc_t) {
        .draw = z9001ui_draw,
        .menus = {
            {
                .name = "System",
                .items = {
                    { .name = "Reset", .func = z9001ui_reset },
                    { .name = "Boot to Z9001", .func = z9001ui_boot_z9001 },
                    { .name = "Boot to KC87", .func = z9001ui_boot_kc87 },
                }
            },
            {
                .name = "Hardware",
                .items = {
                    { .name = "Memory Map", .func = z9001ui_memmap_toggle },
                    { .name = "Z80 CPU", .func = z9001ui_cpu_toggle },
                    { .name = "Z80 PIO 1", .func = z9001ui_pio1_toggle },
                    { .name = "Z80 PIO 2", .func = z9001ui_pio2_toggle },
                    { .name = "Z80 CTC", .func = z9001ui_ctc_toggle },
                }
            },
            {
                .name = "Debug",
                .items = {
                    { .name = "Memory Editor", .func = z9001ui_memedit_toggle },
                    { .name = "Disassembler", .func = z9001ui_dasm_toggle },
                    { .name = "CPU Debugger (TODO)", .func = z9001ui_dummy },
                    { .name = "Scan Commands (TODO)", .func = z9001ui_dummy }
                }
            }
        },
    });
    ui_memedit_init(&ui_memedit, &(ui_memedit_desc_t){
        .title = "Memory Editor",
        .layers = { "System" },
        .read_cb = z9001ui_mem_read,
        .write_cb = z9001ui_mem_write,
        .read_only = false,
        .x = 20, .y = 40, .h = 120
    });
    ui_memmap_init(&ui_memmap, &(ui_memmap_desc_t){
        .title = "Memory Map",
        .x = 30, .y = 50, .w = 400, .h = 64
    });
    ui_dasm_init(&ui_dasm, &(ui_dasm_desc_t){
        .title = "Disassembler",
        .layers = { "System" },
        .start_addr = 0xF000,
        .read_cb = z9001ui_mem_read,
        .x = 40, .y = 60, .w = 400, .h = 256
    });
    ui_z80pio_init(&ui_pio_1, &(ui_z80pio_desc_t){
        .title = "Z80 PIO 1",
        .pio = &z9001.pio1,
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
    ui_z80pio_init(&ui_pio_2, &(ui_z80pio_desc_t){
        .title = "Z80 PIO 2",
        .pio = &z9001.pio2,
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
        .ctc = &z9001.ctc,
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
        .cpu = &z9001.cpu,
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

void z9001ui_discard(void) {
    ui_z80ctc_discard(&ui_ctc);
    ui_z80pio_discard(&ui_pio_1);
    ui_z80pio_discard(&ui_pio_2);
    ui_z80_discard(&ui_cpu);
    ui_dasm_discard(&ui_dasm);
    ui_memmap_discard(&ui_memmap);
    ui_memedit_discard(&ui_memedit);
}

void z9001ui_update_memmap(void) {
    ui_memmap_reset(&ui_memmap);
    ui_memmap_layer(&ui_memmap, "System");
    if (Z9001_TYPE_Z9001 == z9001.type) {
        /* Z9001 memory map */
        ui_memmap_region(&ui_memmap, "RAM", 0x0000, 0x4000, true);
        ui_memmap_region(&ui_memmap, "16KB RAM MODULE", 0x4000, 0x4000, true);
        if (z9001.z9001_has_basic_rom) {
            ui_memmap_region(&ui_memmap, "BASIC ROM MODULE", 0xC000, 0x2800, true);
        }
        ui_memmap_region(&ui_memmap, "ASCII RAM", 0xEC00, 0x0400, true);
        ui_memmap_region(&ui_memmap, "OS ROM 1", 0xF000, 0x0800, true);
        ui_memmap_region(&ui_memmap, "OS ROM 2", 0xF800, 0x0800, true);
    }
    else {
        /* KC87 memory map */
        ui_memmap_region(&ui_memmap, "RAM", 0x0000, 0xC000, true);
        ui_memmap_region(&ui_memmap, "BASIC ROM", 0xC000, 0x2000, true);
        ui_memmap_region(&ui_memmap, "OS ROM 1", 0xE000, 0x0800, true);
        ui_memmap_region(&ui_memmap, "COLOR RAM", 0xE800, 0x0400, true);
        ui_memmap_region(&ui_memmap, "ASCII RAM", 0xEC00, 0x0400, true);
        ui_memmap_region(&ui_memmap, "OS ROM 2", 0xF000, 0x1000, true);
    }
}

void z9001ui_draw() {
    if (ui_memmap_isopen(&ui_memmap)) {
        z9001ui_update_memmap();
    }
    ui_z80_draw(&ui_cpu);
    ui_z80pio_draw(&ui_pio_1);
    ui_z80pio_draw(&ui_pio_2);
    ui_z80ctc_draw(&ui_ctc);
    ui_memedit_draw(&ui_memedit);
    ui_memmap_draw(&ui_memmap);
    ui_dasm_draw(&ui_dasm);
}
#endif /* CHIPS_USE_UI */
