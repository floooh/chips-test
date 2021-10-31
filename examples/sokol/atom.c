/*
    atom.c

    The Acorn Atom was a very simple 6502-based home computer
    (just a MOS 6502 CPU, Motorola MC6847 video
    display generator, and Intel i8255 I/O chip).

    Note: Ctrl+L (clear screen) is mapped to F1.

    NOT EMULATED:
        - REPT key (and some other special keys)
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/mc6847.h"
#include "chips/i8255.h"
#include "chips/m6522.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/atom.h"
#include "atom-roms.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_M6502
    #include "ui.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_m6502.h"
    #include "ui/ui_m6522.h"
    #include "ui/ui_mc6847.h"
    #include "ui/ui_i8255.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_kbd.h"
    #include "ui/ui_atom.h"
#endif

static struct {
    atom_t atom;
    uint32_t frame_time_us;
    uint32_t ticks;
    double exec_time_ms;
    #ifdef CHIPS_USE_UI
        ui_atom_t ui_atom;
    #endif
} state;

#ifdef CHIPS_USE_UI
#define BORDER_TOP (24)
#else
#define BORDER_TOP (8)
#endif
#define BORDER_LEFT (8)
#define BORDER_RIGHT (8)
#define BORDER_BOTTOM (16)

static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

atom_desc_t atom_desc(atom_joystick_type_t joy_type) {
    return (atom_desc_t) {
        .joystick_type = joy_type,
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_abasic = dump_abasic_ic20,
        .rom_abasic_size = sizeof(dump_abasic_ic20),
        .rom_afloat = dump_afloat_ic21,
        .rom_afloat_size = sizeof(dump_afloat_ic21),
        .rom_dosrom = dump_dosrom_u15,
        .rom_dosrom_size = sizeof(dump_dosrom_u15),
        #if defined(CHIPS_USE_UI)
        .debug = ui_atom_get_debug(&state.ui_atom)
        #endif
    };
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(void) {
    ui_atom_draw(&state.ui_atom);
}
static void ui_boot_cb(atom_t* sys) {
    atom_desc_t desc = atom_desc(sys->joystick_type);
    atom_init(sys, &desc);
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
    fs_init();
    saudio_setup(&(saudio_desc){0});
    atom_joystick_type_t joy_type = ATOM_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        if (sargs_equals("joystick", "mmc") || sargs_equals("joystick", "yes")) {
            joy_type = ATOM_JOYSTICKTYPE_MMC;
        }
    }
    atom_desc_t desc = atom_desc(joy_type);
    atom_init(&state.atom, &desc);
    #ifdef CHIPS_USE_UI
        ui_init(ui_draw_cb);
        ui_atom_init(&state.ui_atom, &(ui_atom_desc_t){
            .atom = &state.atom,
            .boot_cb = ui_boot_cb,
            .create_texture_cb = gfx_create_texture,
            .update_texture_cb = gfx_update_texture,
            .destroy_texture_cb = gfx_destroy_texture,
            .dbg_keys = {
                .cont = { .keycode = SAPP_KEYCODE_F5, .name = "F5" },
                .stop = { .keycode = SAPP_KEYCODE_F5, .name = "F5" },
                .step_over = { .keycode = SAPP_KEYCODE_F6, .name = "F6" },
                .step_into = { .keycode = SAPP_KEYCODE_F7, .name = "F7" },
                .step_tick = { .keycode = SAPP_KEYCODE_F8, .name = "F8" },
                .toggle_breakpoint = { .keycode = SAPP_KEYCODE_F9, .name = "F9" }
            }
        });
    #endif
    bool delay_input = false;
    if (sargs_exists("file")) {
        delay_input = true;
        fs_start_load_file(sargs_value("file"));
    }
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

static void handle_file_loading(void);
static void send_keybuf_input(void);
static void draw_status_bar(void);

void app_frame() {
    state.frame_time_us = clock_frame_time();
    const uint64_t exec_start_time = stm_now();
    state.ticks = atom_exec(&state.atom, state.frame_time_us);
    state.exec_time_ms = stm_ms(stm_since(exec_start_time));
    draw_status_bar();
    gfx_draw(atom_display_width(&state.atom), atom_display_height(&state.atom));
    handle_file_loading();
    send_keybuf_input();
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        /* input was handled by UI */
        return;
    }
    #endif
    int c = 0;
    switch (event->type) {
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
                atom_key_down(&state.atom, c);
                atom_key_up(&state.atom, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_UP:
        case SAPP_EVENTTYPE_KEY_DOWN:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = 0x20; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                case SAPP_KEYCODE_UP:           c = 0x0B; break;
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_INSERT:       c = 0x1A; break;
                case SAPP_KEYCODE_HOME:         c = 0x19; break;
                case SAPP_KEYCODE_BACKSPACE:    c = 0x01; break;
                case SAPP_KEYCODE_ESCAPE:       c = 0x1B; break;
                case SAPP_KEYCODE_F1:           c = 0x0C; break; /* mapped to Ctrl+L (clear screen) */
                default:                        c = 0;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    atom_key_down(&state.atom, c);
                }
                else {
                    atom_key_up(&state.atom, c);
                }
            }
            break;
        case SAPP_EVENTTYPE_FILES_DROPPED:
            fs_start_load_dropped_file();
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    atom_discard(&state.atom);
    #ifdef CHIPS_USE_UI
        ui_atom_discard(&state.ui_atom);
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        atom_key_down(&state.atom, key_code);
        atom_key_up(&state.atom, key_code);
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = 48;
    if (fs_ptr() && clock_frame_count_60hz() > load_delay_frames) {
        bool load_success = false;
        if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        if (fs_ext("tap")) {
            load_success = atom_insert_tape(&state.atom, fs_ptr(), fs_size());
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
        fs_free();
    }
}

static void draw_status_bar(void) {
    const float w = sapp_widthf();
    const float h = sapp_heightf();
    double frame_time_ms = state.frame_time_us / 1000.0f;
    sdtx_canvas(w, h);
    sdtx_color3b(255, 255, 255);
    sdtx_pos(1.0f, (h / 8.0f) - 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms ticks/frame:%d", frame_time_ms, state.exec_time_ms, state.ticks);
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * atom_std_display_width() + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * atom_std_display_height() + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "Acorn Atom",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
    };
}

