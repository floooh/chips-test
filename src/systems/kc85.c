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
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_Z80
    #include "ui.h"
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
#endif

static struct {
    kc85_t kc85;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    kc85_module_type_t delay_insert_module; // module to insert after ROM module image has been loaded
    #ifdef CHIPS_USE_UI
        ui_kc85_t ui_kc85;
    #endif
} state;

#ifdef CHIPS_USE_UI
#define BORDER_TOP (24)
#else
#define BORDER_TOP (8)
#endif
#define BORDER_LEFT (8)
#define BORDER_RIGHT (8)
#define BORDER_BOTTOM (32)

#if defined(CHIPS_KC85_TYPE_2)
#define KC85_TYPE_NAME "KC85/2"
#define LOAD_DELAY_FRAMES (480)
#elif defined(CHIPS_KC85_TYPE_3)
#define KC85_TYPE_NAME "KC85/3"
#define LOAD_DELAY_FRAMES (480)
#else
#define KC85_TYPE_NAME "KC85/4"
#define LOAD_DELAY_FRAMES (180)
#endif

// audio-streaming callback
static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

// a callback to patch some known problems in game snapshot files
static void patch_snapshots(const char* snapshot_name, void* user_data) {
    (void)user_data;
    if (strcmp(snapshot_name, "JUNGLE     ") == 0) {
        /* patch start level 1 into memory */
        mem_wr(&state.kc85.mem, 0x36b7, 1);
        mem_wr(&state.kc85.mem, 0x3697, 1);
        for (int i = 0; i < 5; i++) {
            mem_wr(&state.kc85.mem, 0x1770 + i, mem_rd(&state.kc85.mem, 0x36b6 + i));
        }
    }
    else if (strcmp(snapshot_name, "DIGGER  COM\x01") == 0) {
        mem_wr16(&state.kc85.mem, 0x09AA, 0x0160);    /* time for delay-loop 0160 instead of 0260 */
        mem_wr(&state.kc85.mem, 0x3d3a, 0xB5);        /* OR L instead of OR (HL) */
    }
    else if (strcmp(snapshot_name, "DIGGERJ") == 0) {
        mem_wr16(&state.kc85.mem, 0x09AA, 0x0260);
        mem_wr(&state.kc85.mem, 0x3d3a, 0xB5);       /* OR L instead of OR (HL) */
    }
}

// get kc85_desc_t struct for a given KC85 type
kc85_desc_t kc85_desc(void) {
    return (kc85_desc_t) {
        .pixel_buffer = { .ptr=gfx_framebuffer(), .size=gfx_framebuffer_size() },
        .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .patch_callback = { .func = patch_snapshots },
        .roms = {
            #if defined(CHIPS_KC85_TYPE_2)
                .caos22 = { .ptr = dump_caos22_852, .size = sizeof(dump_caos22_852) },
            #elif defined(CHIPS_KC85_TYPE_3)
                .caos31 = { .ptr = dump_caos31_853, .size = sizeof(dump_caos31_853) },
            #elif defined(CHIPS_KC85_TYPE_4)
                .caos42c = { .ptr = dump_caos42c_854, .size = sizeof(dump_caos42c_854) },
                .caos42e = { .ptr = dump_caos42e_854, .size = sizeof(dump_caos42e_854) },
            #endif
            #if !defined(CHIPS_KC85_TYPE_2)
                .kcbasic = { .ptr = dump_basic_c0_853, .size = sizeof(dump_basic_c0_853) }
            #endif
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_kc85_get_debug(&state.ui_kc85)
        #endif
    };
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(void) {
    ui_kc85_draw(&state.ui_kc85);
}
static void ui_boot_cb(kc85_t* sys) {
    kc85_desc_t desc = kc85_desc();
    kc85_init(sys, &desc);
}
#endif

void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .border_left = BORDER_LEFT,
        .border_right = BORDER_RIGHT,
        .border_top = BORDER_TOP,
        .border_bottom = BORDER_BOTTOM,
    });
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames = 10 });
    clock_init();
    prof_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    const kc85_desc_t desc = kc85_desc();
    kc85_init(&state.kc85, &desc);
    #ifdef CHIPS_USE_UI
        ui_init(ui_draw_cb);
        ui_kc85_init(&state.ui_kc85, &(ui_kc85_desc_t){
            .kc85 = &state.kc85,
            .boot_cb = ui_boot_cb,
            .create_texture_cb = gfx_create_texture,
            .update_texture_cb = gfx_update_texture,
            .destroy_texture_cb = gfx_destroy_texture,
            .dbg_keys = {
                .cont = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F5), .name = "F5" },
                .stop = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F5), .name = "F5" },
                .step_over = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F6), .name = "F6" },
                .step_into = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F7), .name = "F7" },
                .step_tick = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F8), .name = "F8" },
                .toggle_breakpoint = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F9), .name = "F9" }
            }
        });
    #endif

    bool delay_input = false;
    // snapshot file or rom-module image
    if (sargs_exists("file")) {
        delay_input=true;
        fs_start_load_file(sargs_value("file"));
    }
    else if (sargs_exists("mod_image")) {
        fs_start_load_file(sargs_value("mod_image"));
    }
    // check if any modules should be inserted
    if (sargs_exists("mod")) {
        /* RAM modules can be inserted immediately, ROM modules
           only after the ROM image has been loaded
        */
        if (sargs_equals("mod", "m022")) {
            kc85_insert_ram_module(&state.kc85, 0x08, KC85_MODULE_M022_16KBYTE);
        }
        else if (sargs_equals("mod", "m011")) {
            kc85_insert_ram_module(&state.kc85, 0x08, KC85_MODULE_M011_64KBYE);
        }
        else {
            // a ROM module
            delay_input = true;
            if (sargs_equals("mod", "m006")) {
                state.delay_insert_module = KC85_MODULE_M006_BASIC;
            }
            else if (sargs_equals("mod", "m012")) {
                state.delay_insert_module = KC85_MODULE_M012_TEXOR;
            }
            else if (sargs_equals("mod", "m026")) {
                state.delay_insert_module = KC85_MODULE_M026_FORTH;
            }
            else if (sargs_equals("mod", "m027")) {
                state.delay_insert_module = KC85_MODULE_M027_DEVELOPMENT;
            }
        }
    }
    // keyboard input to send to emulator
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

static void handle_file_loading(void);
static void send_keybuf_input(void);
static void draw_status_bar(void);

void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t emu_start_time = stm_now();
    state.ticks = kc85_exec(&state.kc85, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(kc85_display_width(&state.kc85), kc85_display_height(&state.kc85));
    handle_file_loading();
    send_keybuf_input();
}

void app_input(const sapp_event* event) {
    // accept dropped files also when ImGui grabs input
    if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
        fs_start_load_dropped_file();
    }
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        // input was handled by UI
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
                kc85_key_down(&state.kc85, c);
                kc85_key_up(&state.kc85, c);
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
                    kc85_key_down(&state.kc85, c);
                }
                else {
                    kc85_key_up(&state.kc85, c);
                }
            }
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    kc85_discard(&state.kc85);
    #ifdef CHIPS_USE_UI
        ui_kc85_discard(&state.ui_kc85);
        ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        kc85_key_down(&state.kc85, key_code);
        kc85_key_up(&state.kc85, key_code);
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = LOAD_DELAY_FRAMES;
    if (fs_ptr() && clock_frame_count_60hz() > load_delay_frames) {
        bool load_success = false;
        if (sargs_exists("mod_image")) {
            // insert the rom module
            if (state.delay_insert_module != KC85_MODULE_NONE) {
                load_success = kc85_insert_rom_module(&state.kc85, 0x08, state.delay_insert_module, fs_ptr(), fs_size());
            }
        }
        else if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        else {
            load_success = kc85_quickload(&state.kc85, fs_ptr(), fs_size());
        }
        if (load_success) {
            if (clock_frame_count_60hz() > (load_delay_frames + 10)) {
                gfx_flash_success();
            }
            if (sargs_exists("input")) {
                keybuf_put(sargs_value("input"));
            }
        }
        else {
            gfx_flash_error();
        }
        fs_reset();
    }
}

static void draw_status_bar(void) {
    prof_push(PROF_EMU, (float)state.emu_time_ms);
    prof_stats_t emu_stats = prof_stats(PROF_EMU);

    const float w = sapp_widthf();
    const float h = sapp_heightf();
    const char* slot_c = kc85_slot_mod_short_name(&state.kc85, 0x0C);
    const char* slot_8 = kc85_slot_mod_short_name(&state.kc85, 0x08);
    const uint8_t pio_a = state.kc85.pio_a;
    const uint32_t text_color = 0xFFFFFFFF;
    const uint32_t green_active = 0xFF00EE00;
    const uint32_t green_inactive = 0xFF006600;
    sdtx_canvas(w, h);
    sdtx_origin(1.0f, (h / 8.0f) - 3.5f);
    sdtx_color1i(text_color);
    sdtx_printf(KC85_TYPE_NAME " SLOTC:%s SLOT8:%s", slot_c, slot_8);

    sdtx_color1i(text_color);
    sdtx_puts("  BASIC: ");
    sdtx_color1i((pio_a & KC85_PIO_A_BASIC_ROM) ? green_active : green_inactive);
    sdtx_putc(0xCF);

    sdtx_color1i(text_color);
    sdtx_puts("  CAOS: ");
    sdtx_color1i((pio_a & KC85_PIO_A_CAOS_ROM) ? green_active : green_inactive);
    sdtx_putc(0xCF);

    sdtx_color1i(text_color);
    sdtx_puts("  RAM: ");
    sdtx_color1i((pio_a & KC85_PIO_A_RAM) ? green_active : green_inactive);
    sdtx_putc(0xCF);

    sdtx_color1i(text_color);
    sdtx_puts("  IRM: ");
    sdtx_color1i((pio_a & KC85_PIO_A_IRM) ? green_active : green_inactive);
    sdtx_putc(0xCF);

    sdtx_pos(0.0f, 1.5f);
    sdtx_color1i(text_color);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", (float)state.frame_time_us * 0.001f, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc) { .argc = argc, .argv = argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * kc85_std_display_width()  + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * kc85_std_display_height() + BORDER_TOP + BORDER_BOTTOM,
        .window_title = KC85_TYPE_NAME,
        .icon.sokol_default = true,
        .enable_dragndrop = true,
    };
}

