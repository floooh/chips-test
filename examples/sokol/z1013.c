/*
    z1013.c

    The Robotron Z1013, see chips/systems/z1013.h for details!

    I have added a pre-loaded KC-BASIC interpreter:

    Start the BASIC interpreter with 'J 300', return to OS with 'BYE'.

    Enter BASIC editing mode with 'AUTO', leave by pressing Esc.
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/z1013.h"
#include "z1013-roms.h"
#ifdef CHIPS_USE_UI
#undef CHIPS_IMPL
#include "ui.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_z80.h"
#include "ui/ui_z80pio.h"
#endif

static z1013_t z1013;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

#ifdef CHIPS_USE_UI
void z1013ui_init(void);
void z1013ui_discard(void);
void z1013ui_draw(void);
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
        .width = 2 * Z1013_DISPLAY_WIDTH,
        .height = 2 * Z1013_DISPLAY_HEIGHT,
        .window_title = "Robotron Z1013",
        .ios_keyboard_resizes_canvas = true
    };
}

/* get z1013_desc_t struct for given Z1013 model */
static z1013_desc_t z1013_desc(z1013_type_t type) {
    return(z1013_desc_t) {
        .type = type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_mon_a2 = dump_z1013_mon_a2,
        .rom_mon_a2_size = sizeof(dump_z1013_mon_a2),
        .rom_mon202 = dump_z1013_mon202,
        .rom_mon202_size = sizeof(dump_z1013_mon202),
        .rom_font = dump_z1013_font,
        .rom_font_size = sizeof(dump_z1013_font)
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(&(gfx_desc_t){
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        .top_offset = 16,
        .fb_width = Z1013_DISPLAY_WIDTH,
        .fb_height = Z1013_DISPLAY_HEIGHT + 16,
        #else
        .fb_width = Z1013_DISPLAY_WIDTH,
        .fb_height = Z1013_DISPLAY_HEIGHT,
        #endif
    });
    #ifdef CHIPS_USE_UI
    z1013ui_init();
    #endif
    clock_init();
    z1013_type_t type = Z1013_TYPE_64;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "z1013_01")) {
            type = Z1013_TYPE_01;
        }
        else if (sargs_equals("type", "z1013_16")) {
            type = Z1013_TYPE_16;
        }
    }
    z1013_desc_t desc = z1013_desc(type);
    z1013_init(&z1013, &desc);
    /* copy BASIC interpreter into RAM, so the user has something to play around 
        with (started with "J 300 [Enter]"), skip first 0x20 bytes .z80 file format header
    */
    if (Z1013_TYPE_64 == z1013.type) {
        mem_write_range(&z1013.mem, 0x0100, dump_kc_basic+0x20, sizeof(dump_kc_basic)-0x20);
    }
}

/* per frame stuff: tick the emulator, render the framebuffer, delay-load game files */
void app_frame(void) {
    #if CHIPS_USE_UI
        uint64_t start = stm_now();
        z1013_exec(&z1013, clock_frame_time());
        ui_set_exec_time(stm_since(start));
    #else
        z1013_exec(&z1013, clock_frame_time());
    #endif
    gfx_draw();
    if (fs_ptr() && clock_frame_count() > 20) {
        z1013_quickload(&z1013, fs_ptr(), fs_size());
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
                z1013_key_down(&z1013, c);
                z1013_key_up(&z1013, c);
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
                case SAPP_KEYCODE_ESCAPE:   c = 0x03; break;
                default:                    c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    z1013_key_down(&z1013, c);
                }
                else {
                    z1013_key_up(&z1013, c);
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
void app_cleanup(void) {
    z1013_discard(&z1013);
    #ifdef CHIPS_USE_UI
    z1013ui_discard();
    #endif
    gfx_shutdown();
}

/*=== optional debugging UI ==================================================*/
#ifdef CHIPS_USE_UI

static ui_memedit_t ui_memedit;
static ui_memmap_t ui_memmap;
static ui_dasm_t ui_dasm;
static ui_z80_t ui_cpu;
static ui_z80pio_t ui_pio;

/* menu handler functions */
void z1013ui_reset(void) { z1013_reset(&z1013); }
void z1013ui_boot_01(void) { z1013_desc_t desc = z1013_desc(Z1013_TYPE_01); z1013_init(&z1013, &desc); }
void z1013ui_boot_16(void) { z1013_desc_t desc = z1013_desc(Z1013_TYPE_16); z1013_init(&z1013, &desc); }
void z1013ui_boot_64(void) { z1013_desc_t desc = z1013_desc(Z1013_TYPE_64); z1013_init(&z1013, &desc); }
void z1013ui_memedit_toggle(void) { ui_memedit_toggle(&ui_memedit); }
void z1013ui_memmap_toggle(void) { ui_memmap_toggle(&ui_memmap); }
void z1013ui_dasm_toggle(void) { ui_dasm_toggle(&ui_dasm); }
void z1013ui_cpu_toggle(void) { ui_z80_toggle(&ui_cpu); }
void z1013ui_pio_toggle(void) { ui_z80pio_toggle(&ui_pio); }

uint8_t z1013ui_mem_read(int layer, uint16_t addr, void* user_data) {
    return mem_rd(&z1013.mem, addr);
}

void z1013ui_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    mem_wr(&z1013.mem, addr, data);
}

void z1013ui_dummy(void) { }

void z1013ui_init(void) {
    ui_init(&(ui_desc_t) {
        .draw = z1013ui_draw,
        .menus = {
            {
                .name = "System",
                .items = {
                    { .name = "Reset", .func = z1013ui_reset },
                    { .name = "Boot to Z1013/01", .func = z1013ui_boot_01 },
                    { .name = "Boot to Z1013/16", .func = z1013ui_boot_16 },
                    { .name = "Boot to Z1013/64", .func = z1013ui_boot_64 }
                }
            },
            {
                .name = "Hardware",
                .items = {
                    { .name = "Memory Map", .func = z1013ui_memmap_toggle },
                    { .name = "Z80 CPU", .func = z1013ui_cpu_toggle },
                    { .name = "Z80 PIO", .func = z1013ui_pio_toggle },
                }
            },
            {
                .name = "Debug",
                .items = {
                    { .name = "Memory Editor", .func = z1013ui_memedit_toggle },
                    { .name = "Disassembler", .func = z1013ui_dasm_toggle },
                    { .name = "CPU Debugger (TODO)", .func = z1013ui_dummy },
                }
            }
        },
    });
    ui_memedit_init(&ui_memedit, &(ui_memedit_desc_t){
        .title = "Memory Editor",
        .layers = { "System" },
        .read_cb = z1013ui_mem_read,
        .write_cb = z1013ui_mem_write,
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
        .read_cb = z1013ui_mem_read,
        .x = 40, .y = 60, .w = 400, .h = 256
    });
    ui_z80pio_init(&ui_pio, &(ui_z80pio_desc_t){
        .title = "Z80 PIO",
        .pio = &z1013.pio,
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
    ui_z80_init(&ui_cpu, &(ui_z80_desc_t){
        .title = "Z80 CPU",
        .cpu = &z1013.cpu,
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

void z1013ui_discard(void) {
    ui_z80pio_discard(&ui_pio);
    ui_z80_discard(&ui_cpu);
    ui_dasm_discard(&ui_dasm);
    ui_memmap_discard(&ui_memmap);
    ui_memedit_discard(&ui_memedit);
}

void z1013ui_update_memmap(void) {
    ui_memmap_reset(&ui_memmap);
    ui_memmap_layer(&ui_memmap, "System");
    if ((Z1013_TYPE_01 == z1013.type) || (Z1013_TYPE_16 == z1013.type)) {
        /* Z1013/01 + /16 memory map */
        ui_memmap_region(&ui_memmap, "RAM", 0x0000, 0x4000, true);
        ui_memmap_region(&ui_memmap, "VIDEO", 0xEC00, 0x0400, true);
        ui_memmap_region(&ui_memmap, "ROM", 0xF000, 0x0800, true);
    }
    else {
        /* Z1013/64 memory map */
        ui_memmap_region(&ui_memmap, "RAM0", 0x0000, 0xEC00, true);
        ui_memmap_region(&ui_memmap, "VIDEO", 0xEC00, 0x0400, true);
        ui_memmap_region(&ui_memmap, "ROM", 0xF000, 0x0800, true);
        ui_memmap_region(&ui_memmap, "RAM1", 0xF800, 0x0800, true);
    }
}

void z1013ui_draw() {
    if (ui_memmap_isopen(&ui_memmap)) {
        z1013ui_update_memmap();
    }
    ui_z80_draw(&ui_cpu);
    ui_z80pio_draw(&ui_pio);
    ui_memedit_draw(&ui_memedit);
    ui_memmap_draw(&ui_memmap);
    ui_dasm_draw(&ui_dasm);
}
#endif /* CHIPS_USE_UI */
