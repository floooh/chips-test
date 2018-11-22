/*
    cpc.c

    Amstrad CPC 464/6128 and KC Compact. No disc emulation.
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/i8255.h"
#include "chips/mc6845.h"
#include "chips/upd765.h"
#include "chips/crt.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/fdd.h"
#include "chips/fdd_cpc.h"
#include "systems/cpc.h"
#include "cpc-roms.h"
#ifdef CHIPS_USE_UI
#undef CHIPS_IMPL
#include "ui.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_z80.h"
#include "ui/ui_ay38910.h"
#include "ui/ui_audio.h"
#endif

cpc_t cpc;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

#ifdef CHIPS_USE_UI
void cpcui_init(void);
void cpcui_discard(void);
void cpcui_draw(void);
#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = CPC_DISPLAY_WIDTH,
        .height = 2 * CPC_DISPLAY_HEIGHT,
        .window_title = "CPC 6128",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* get cpc_desc_t struct based on model and joystick type */
cpc_desc_t cpc_desc(cpc_type_t type, cpc_joystick_type_t joy_type) {
    return (cpc_desc_t) {
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .rom_464_os = dump_cpc464_os,
        .rom_464_os_size = sizeof(dump_cpc464_os),
        .rom_464_basic = dump_cpc464_basic,
        .rom_464_basic_size = sizeof(dump_cpc464_basic),
        .rom_6128_os = dump_cpc6128_os,
        .rom_6128_os_size = sizeof(dump_cpc6128_os),
        .rom_6128_basic = dump_cpc6128_basic,
        .rom_6128_basic_size = sizeof(dump_cpc6128_basic),
        .rom_6128_amsdos = dump_cpc6128_amsdos,
        .rom_6128_amsdos_size = sizeof(dump_cpc6128_amsdos),
        .rom_kcc_os = dump_kcc_os,
        .rom_kcc_os_size = sizeof(dump_kcc_os),
        .rom_kcc_basic = dump_kcc_bas,
        .rom_kcc_basic_size = sizeof(dump_kcc_bas)
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(&(gfx_desc_t){
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        .top_offset = 16,
        .fb_width = CPC_DISPLAY_WIDTH,
        .fb_height = CPC_DISPLAY_HEIGHT + 16,
        #else
        .fb_width = CPC_DISPLAY_WIDTH,
        .fb_height = CPC_DISPLAY_HEIGHT,
        #endif
        .aspect_y = 2
    });
    keybuf_init(10);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    bool delay_input = false;
    if (sargs_exists("file")) {
        fs_load_file(sargs_value("file"));
    }
    if (sargs_exists("tape")) {
        fs_load_file(sargs_value("tape"));
    }
    if (sargs_exists("disc")) {
        delay_input = true;
        fs_load_file(sargs_value("disc"));
    }
    cpc_type_t type = CPC_TYPE_6128;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "cpc464")) {
            type = CPC_TYPE_464;
        }
        else if (sargs_equals("type", "kccompact")) {
            type = CPC_TYPE_KCCOMPACT;
        }
    }
    cpc_joystick_type_t joy_type = CPC_JOYSTICK_NONE;
    if (sargs_exists("joystick")) {
        joy_type = CPC_JOYSTICK_DIGITAL;
    }
    cpc_desc_t desc = cpc_desc(type, joy_type);
    cpc_init(&cpc, &desc);
    #ifdef CHIPS_USE_UI
    cpcui_init();
    #endif

    /* keyboard input to send to emulator */
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame(void) {
    #if CHIPS_USE_UI
        uint64_t start = stm_now();
        cpc_exec(&cpc, clock_frame_time());
        ui_set_exec_time(stm_since(start));
    #else
        cpc_exec(&cpc, clock_frame_time());
    #endif
    gfx_draw();
    /* load a data file? FIXME: this needs proper file type detection in case
       a file has been dragged'n'dropped
    */
    if (fs_ptr() && clock_frame_count() > 120) {
        if (sargs_exists("tape")) {
            /* load the file data via the tape-emulation (TAP format) */
            if (cpc_insert_tape(&cpc, fs_ptr(), fs_size())) {
                /* issue the right commands to start loading from tape */
                keybuf_put("|tape\nrun\"\n\n");
            }
        }
        else if (sargs_exists("disc")) {
            if (cpc_insert_disc(&cpc, fs_ptr(), fs_size())) {
                if (sargs_exists("input")) {
                    keybuf_put(sargs_value("input"));
                }
            }
        }
        else {
            /* load the file data as a quickload-snapshot (SNA or BIN format)*/
            cpc_quickload(&cpc, fs_ptr(), fs_size());
        }
        fs_free();
    }
    uint8_t key_code;
    if (0 != (key_code = keybuf_get())) {
        cpc_key_down(&cpc, key_code);
        cpc_key_up(&cpc, key_code);
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
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                cpc_key_down(&cpc, c);
                cpc_key_up(&cpc, c);
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
                case SAPP_KEYCODE_LEFT_SHIFT:   c = 0x02; break;
                case SAPP_KEYCODE_BACKSPACE:    c = shift ? 0x0C : 0x01; break; // 0x0C: clear screen
                case SAPP_KEYCODE_ESCAPE:       c = shift ? 0x13 : 0x03; break; // 0x13: break
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
                    cpc_key_down(&cpc, c);
                }
                else {
                    cpc_key_up(&cpc, c);
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
    cpc_discard(&cpc);
    #ifdef CHIPS_USE_UI
    cpcui_discard();
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
static ui_audio_t ui_audio;

/* menu handler functions */
void cpcui_reset(void) { cpc_reset(&cpc); }
void cpcui_boot_6128(void) { cpc_desc_t desc = cpc_desc(CPC_TYPE_6128, cpc.joystick_type); cpc_init(&cpc, &desc); }
void cpcui_boot_464(void) { cpc_desc_t desc = cpc_desc(CPC_TYPE_464, cpc.joystick_type); cpc_init(&cpc, &desc); }
void cpcui_boot_kcc(void) { cpc_desc_t desc = cpc_desc(CPC_TYPE_KCCOMPACT, cpc.joystick_type); cpc_init(&cpc, &desc); }

uint8_t cpcui_mem_read(int layer, uint16_t addr, void* user_data) {
    if (0 == layer) {
        return mem_rd(&cpc.mem, addr);
    }
    else {
        // FIXME
        return 0;
    }
}

void cpcui_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    if (0 == layer) {
        return mem_wr(&cpc.mem, addr, data);
    }
    else {
        // FIXME
    }
}

void cpcui_dummy(void) { }

void cpcui_init(void) {
    ui_init(&(ui_desc_t) {
        .draw = cpcui_draw,
        .menus = {
            {
                .name = "System",
                .items = {
                    { .name = "Reset", .func = cpcui_reset },
                    { .name = "CPC 464", .func = cpcui_boot_464 },
                    { .name = "CPC 6128", .func = cpcui_boot_6128 },
                    { .name = "KC Compact", .func = cpcui_boot_kcc },
                }
            },
            {
                .name = "Hardware",
                .items = {
                    { .name = "Memory Map", .open = &ui_memmap.open },
                    { .name = "System State (TODO)", .func = cpcui_dummy },
                    { .name = "Audio Output", .open = &ui_audio.open },
                    { .name = "Z80 CPU", .open = &ui_cpu.open },
                    { .name = "AY-3-8912", .open = &ui_ay.open },
                    { .name = "MC6845 (TODO)", .func = cpcui_dummy },
                    { .name = "i8255 (TODO)", .func = cpcui_dummy },
                    { .name = "uPD 765 (TODO)", .func = cpcui_dummy },
                }
            },
            {
                .name = "Debug",
                .items = {
                    { .name = "Memory Editor", .open = &ui_memedit.open },
                    { .name = "Disassembler", .open = &ui_dasm.open },
                    { .name = "CPU Debugger (TODO)", .func = cpcui_dummy },
                }
            }
        },
    });
    ui_memedit_init(&ui_memedit, &(ui_memedit_desc_t){
        .title = "Memory Editor",
        .layers = { "CPU Mapped", "FIXME" },
        .read_cb = cpcui_mem_read,
        .write_cb = cpcui_mem_write,
        .x = 20, .y = 40, .h = 120
    });
    ui_memmap_init(&ui_memmap, &(ui_memmap_desc_t){
        .title = "Memory Map",
        .x = 30, .y = 50, .w = 400, .h = 64
    });
    ui_dasm_init(&ui_dasm, &(ui_dasm_desc_t){
        .title = "Disassembler",
        .layers = { "CPU Mapped", "FIXME" },
        .start_addr = 0x0000,
        .read_cb = cpcui_mem_read,
        .x = 40, .y = 60, .w = 400, .h = 256
    });
    ui_audio_init(&ui_audio, &(ui_audio_desc_t) {
        .title = "Audio Output",
        .sample_buffer = cpc.sample_buffer,
        .num_samples = cpc.num_samples,
        .x = 40, .y = 60
    });

    ui_z80_init(&ui_cpu, &(ui_z80_desc_t){
        .title = "Z80 CPU",
        .cpu = &cpc.cpu,
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
        .ay = &cpc.psg,
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

void cpcui_discard(void) {
    ui_audio_discard(&ui_audio);
    ui_z80_discard(&ui_cpu);
    ui_ay38910_discard(&ui_ay);
    ui_dasm_discard(&ui_dasm);
    ui_memmap_discard(&ui_memmap);
    ui_memedit_discard(&ui_memedit);
}

void cpcui_update_memmap(void) {
    ui_memmap_reset(&ui_memmap);
    ui_memmap_layer(&ui_memmap, "System");
    ui_memmap_region(&ui_memmap, "FIXME FIXME FIXME", 0x0000, 0x10000, true);
}

void cpcui_draw() {
    if (ui_memmap.open) {
        cpcui_update_memmap();
    }
    ui_audio_draw(&ui_audio, cpc.sample_pos);
    ui_ay38190_draw(&ui_ay);
    ui_z80_draw(&ui_cpu);
    ui_memedit_draw(&ui_memedit);
    ui_memmap_draw(&ui_memmap);
    ui_dasm_draw(&ui_dasm);
}
#endif /* CHIPS_USE_UI */




