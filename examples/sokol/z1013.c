/*
    z1013.c

    The Robotron Z1013, see chips/systems/z1013.h for details!
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80x.h"
#include "chips/z80pio.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/z1013.h"
#include "z1013-roms.h"
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
    #include "ui/ui_z1013.h"
#endif

static struct {
    z1013_t z1013;
    uint32_t frame_time_us;
    uint32_t ticks;
    double exec_time_ms;
    #ifdef CHIPS_USE_UI
        ui_z1013_t ui_z1013;
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

z1013_desc_t z1013_desc(z1013_type_t type) {
    return(z1013_desc_t) {
        .type = type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_mon_a2 = dump_z1013_mon_a2_bin,
        .rom_mon_a2_size = sizeof(dump_z1013_mon_a2_bin),
        .rom_mon202 = dump_z1013_mon202_bin,
        .rom_mon202_size = sizeof(dump_z1013_mon202_bin),
        .rom_font = dump_z1013_font_bin,
        .rom_font_size = sizeof(dump_z1013_font_bin),
        #if defined(CHIPS_USE_UI)
        .debug = ui_z1013_get_debug(&state.ui_z1013)
        #endif
    };
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(void) {
    ui_z1013_draw(&state.ui_z1013);
}
static void ui_boot_cb(z1013_t* sys, z1013_type_t type) {
    z1013_desc_t desc = z1013_desc(type);
    z1013_init(sys, &desc);
}
#endif

void app_init(void) {
    gfx_init(&(gfx_desc_t){
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .border_left = BORDER_LEFT,
        .border_right = BORDER_RIGHT,
        .border_top = BORDER_TOP,
        .border_bottom = BORDER_BOTTOM,
    });
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames = 6 });
    clock_init();
    fs_init();
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
    z1013_init(&state.z1013, &desc);
    #ifdef CHIPS_USE_UI
        ui_init(ui_draw_cb);
        ui_z1013_init(&state.ui_z1013, &(ui_z1013_desc_t){
            .z1013 = &state.z1013,
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

void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t exec_start_time = stm_now();
    state.ticks = z1013_exec(&state.z1013, state.frame_time_us);
    state.exec_time_ms = stm_ms(stm_since(exec_start_time));
    draw_status_bar();
    gfx_draw(z1013_display_width(&state.z1013), z1013_display_height(&state.z1013));
    handle_file_loading();
    send_keybuf_input();
}

void app_input(const sapp_event* event) {
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        // input was handled by UI
        return;
    }
    #endif
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c >= 0x20) && (c < 0x7F)) {
                // need to invert case (unshifted is upper caps, shifted is lower caps)
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                z1013_key_down(&state.z1013, c);
                z1013_key_up(&state.z1013, c);
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
                    z1013_key_down(&state.z1013, c);
                }
                else {
                    z1013_key_up(&state.z1013, c);
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
    z1013_discard(&state.z1013);
    #ifdef CHIPS_USE_UI
        ui_z1013_discard(&state.ui_z1013);
    #endif
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        z1013_key_down(&state.z1013, key_code);
        z1013_key_up(&state.z1013, key_code);
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = 20;
    if (fs_ptr() && (clock_frame_count_60hz() > load_delay_frames)) {
        bool load_success = false;
        if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        else {
            load_success = z1013_quickload(&state.z1013, fs_ptr(), fs_size());
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
        .width = 2 * z1013_std_display_width() + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * z1013_std_display_height() + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "Robotron Z1013",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
    };
}
