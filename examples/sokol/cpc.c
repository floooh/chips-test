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
#include "chips/am40010.h"
#include "chips/upd765.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/fdd.h"
#include "chips/fdd_cpc.h"
#include "systems/cpc.h"
#include "cpc-roms.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_Z80
    #include "ui.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_z80.h"
    #include "ui/ui_ay38910.h"
    #include "ui/ui_mc6845.h"
    #include "ui/ui_am40010.h"
    #include "ui/ui_i8255.h"
    #include "ui/ui_upd765.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_kbd.h"
    #include "ui/ui_fdd.h"
    #include "ui/ui_cpc.h"
#endif

static struct {
    cpc_t cpc;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #if defined(CHIPS_USE_UI)
        ui_cpc_t ui_cpc;
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

// audio-streaming callback
static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

// get cpc_desc_t struct based on model and joystick type
cpc_desc_t cpc_desc(cpc_type_t type, cpc_joystick_type_t joy_type) {
    return (cpc_desc_t) {
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = { .ptr=gfx_framebuffer(), .size=gfx_framebuffer_size() },
        .audio = {
            .callback = { .func=push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .roms = {
            .cpc464 = {
                .os = { .ptr=dump_cpc464_os_bin, .size=sizeof(dump_cpc464_os_bin) },
                .basic = { .ptr=dump_cpc464_basic_bin, .size=sizeof(dump_cpc464_basic_bin) },
            },
            .cpc6128 = {
                .os = { .ptr=dump_cpc6128_os_bin, .size=sizeof(dump_cpc6128_os_bin) },
                .basic = { .ptr=dump_cpc6128_basic_bin, .size= sizeof(dump_cpc6128_basic_bin) },
                .amsdos = { .ptr=dump_cpc6128_amsdos_bin, .size=sizeof(dump_cpc6128_amsdos_bin) }
            },
            .kcc = {
                .os = { .ptr=dump_kcc_os_bin, .size=sizeof(dump_kcc_os_bin) },
                .basic = { .ptr=dump_kcc_bas_bin, .size=sizeof(dump_kcc_bas_bin) }
            },
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_cpc_get_debug(&state.ui_cpc),
        #endif
    };
}

#if defined(CHIPS_USE_UI)
void ui_draw_cb(void) {
    ui_cpc_draw(&state.ui_cpc);
}
static void ui_boot_cb(cpc_t* sys, cpc_type_t type) {
    cpc_desc_t desc = cpc_desc(type, sys->joystick_type);
    cpc_init(sys, &desc);
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
        .emu_aspect_y = 2
    });
    keybuf_init(&(keybuf_desc_t) { .key_delay_frames=7 });
    clock_init();
    prof_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
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
    cpc_init(&state.cpc, &desc);
    #ifdef CHIPS_USE_UI
        ui_init(ui_draw_cb);
        ui_cpc_init(&state.ui_cpc, &(ui_cpc_desc_t){
            .cpc = &state.cpc,
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
    const uint64_t emu_start_time = stm_now();
    state.ticks = cpc_exec(&state.cpc, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(cpc_display_width(&state.cpc), cpc_display_height(&state.cpc));
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
                cpc_key_down(&state.cpc, c);
                cpc_key_up(&state.cpc, c);
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
                    cpc_key_down(&state.cpc, c);
                }
                else {
                    cpc_key_up(&state.cpc, c);
                }
            }
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    cpc_discard(&state.cpc);
    #ifdef CHIPS_USE_UI
        ui_cpc_discard(&state.ui_cpc);
        ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        cpc_key_down(&state.cpc, key_code);
        cpc_key_up(&state.cpc, key_code);
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = 120;
    if (fs_ptr() && ((clock_frame_count_60hz() > load_delay_frames) || fs_ext("sna"))) {
        bool load_success = false;
        if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        /*
        else if (fs_ext("tap")) {
            load_success = cpc_insert_tape(&state.cpc, fs_ptr(), fs_size());
        }
        */
        else if (fs_ext("dsk")) {
            load_success = cpc_insert_disc(&state.cpc, fs_ptr(), fs_size());
        }
        else if (fs_ext("sna") || fs_ext("bin")) {
            load_success = cpc_quickload(&state.cpc, fs_ptr(), fs_size());
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
    prof_push(PROF_FRAME, (float)state.frame_time_us * 0.001f);
    prof_push(PROF_EMU, (float)state.emu_time_ms);
    prof_stats_t frame_stats = prof_stats(PROF_FRAME);
    prof_stats_t emu_stats = prof_stats(PROF_EMU);
    
    const uint32_t text_color = 0xFFFFFFFF;
    const uint32_t disc_active = 0xFF00EE00;
    const uint32_t disc_inactive = 0xFF006600;
    const uint32_t motor_active = 0xFF00CCEE;
    const uint32_t motor_inactive = 0xFF004466;
    const uint32_t joy_active = 0xFFFFEE00;
    const uint32_t joy_inactive = 0xFF886600;
    
    const float w = sapp_widthf();
    const float h = sapp_heightf();
    sdtx_canvas(w, h);
    sdtx_origin(1.0f, (h / 8.0f) - 3.5f);
    sdtx_font(0);

    // joystick state
    sdtx_puts("JOYSTICK: ");
    sdtx_font(1);
    const uint8_t joymask = cpc_joystick_mask(&state.cpc);
    sdtx_color1i((joymask & CPC_JOYSTICK_LEFT) ? joy_active : joy_inactive);
    sdtx_putc(0x88); // arrow left
    sdtx_color1i((joymask & CPC_JOYSTICK_RIGHT) ? joy_active : joy_inactive);
    sdtx_putc(0x89); // arrow right
    sdtx_color1i((joymask & CPC_JOYSTICK_UP) ? joy_active : joy_inactive);
    sdtx_putc(0x8B); // arrow up
    sdtx_color1i((joymask & CPC_JOYSTICK_DOWN) ? joy_active : joy_inactive);
    sdtx_putc(0x8A); // arrow down
    sdtx_color1i((joymask & CPC_JOYSTICK_BTN0) ? joy_active : joy_inactive);
    sdtx_putc(0x87); // btn
    sdtx_font(0);

    // FDD disc inserted LED
    sdtx_color1i(text_color);
    sdtx_puts("  DISC: ");
    sdtx_color1i(cpc_disc_inserted(&state.cpc) ? disc_active : disc_inactive);
    sdtx_putc(0xCF);    // filled circle
    // FDD motor on LED
    sdtx_color1i(text_color);
    sdtx_puts("  MOTOR: ");
    sdtx_color1i(state.cpc.fdd.motor_on ? motor_active : motor_inactive);
    sdtx_putc(0xCF);
    
    sdtx_color1i(text_color);
    sdtx_printf("  TRACK:%d", state.cpc.fdd.cur_track_index);
    
    sdtx_font(0);
    sdtx_color1i(text_color);
    sdtx_pos(0.0f, 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", frame_stats.avg_val, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = cpc_std_display_width() + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * cpc_std_display_height() + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "CPC",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
    };
}
