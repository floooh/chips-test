/*
    nes.c

    NES.
*/
#include <stdio.h>
#include <math.h>
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "common.h"
#include "chips/clk.h"
#include "chips/m6502.h"
#include "chips/r2c02.h"
#include "systems/nes.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_M6502
    #include "ui.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_m6502.h"
    #include "ui/ui_nes.h"
#endif

static struct {
    nes_t nes;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #if defined(CHIPS_USE_UI)
        ui_nes_t ui;
    #endif
} state;

#ifdef CHIPS_USE_UI
static void ui_draw_cb(void);
#endif

static void draw_status_bar(void);

// audio-streaming callback
static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

static void app_init(void) {
    nes_init(&state.nes, &(nes_desc_t) {
         .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_nes_get_debug(&state.ui)
        #endif
    });
    gfx_init(&(gfx_desc_t){
    #ifdef CHIPS_USE_UI
    .draw_extra_cb = ui_draw,
    #endif
    .display_info = nes_display_info(&state.nes),
    });
    clock_init();
    prof_init();
    saudio_setup(&(saudio_desc){
        .logger.func = slog_func,
    });
    fs_init();

#ifdef CHIPS_USE_UI
    ui_init(ui_draw_cb);
    ui_nes_init(&state.ui, &(ui_nes_desc_t){
        .nes = &state.nes,
        .dbg_texture = {
            .create_cb = gfx_create_texture,
            .update_cb = gfx_update_texture,
            .destroy_cb = gfx_destroy_texture,
        },
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

    if (sargs_exists("file")) {
        fs_start_load_file(FS_SLOT_IMAGE, sargs_value("file"));
    }
}

static void handle_file_loading(void);

static void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t emu_start_time = stm_now();
    state.ticks = nes_exec(&state.nes, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(nes_display_info(&state.nes));
    handle_file_loading();
}

static void app_cleanup(void) {
    nes_discard(&state.nes);
    #ifdef CHIPS_USE_UI
        ui_nes_discard(&state.ui);
        ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void draw_status_bar(void) {
    prof_push(PROF_EMU, (float)state.emu_time_ms);
    prof_stats_t emu_stats = prof_stats(PROF_EMU);

    const uint32_t text_color = 0xFFFFFFFF;
    const uint32_t cart_active = 0xFF00EE00;
    const uint32_t cart_inactive = 0xFF006600;
    const uint32_t pad_active = 0xFFFFEE00;
    const uint32_t pad_inactive = 0xFF886600;

    const float w = sapp_widthf();
    const float h = sapp_heightf();
    sdtx_canvas(w, h);
    sdtx_origin(1.0f, (h / 8.0f) - 3.5f);

    sdtx_puts("PAD: ");
    sdtx_font(1);
    const uint8_t padmask = nes_pad_mask(&state.nes);
    sdtx_color1i((padmask & NES_PAD_LEFT) ? pad_active : pad_inactive);
    sdtx_putc(0x88); // arrow left
    sdtx_color1i((padmask & NES_PAD_RIGHT) ? pad_active : pad_inactive);
    sdtx_putc(0x89); // arrow right
    sdtx_color1i((padmask & NES_PAD_UP) ? pad_active : pad_inactive);
    sdtx_putc(0x8B); // arrow up
    sdtx_color1i((padmask & NES_PAD_DOWN) ? pad_active : pad_inactive);
    sdtx_putc(0x8A); // arrow down
    sdtx_color1i((padmask & NES_PAD_START) ? pad_active : pad_inactive);
    sdtx_putc(0x87); // btn
    sdtx_color1i((padmask & NES_PAD_SEL) ? pad_active : pad_inactive);
    sdtx_putc(0x87); // btn
    sdtx_color1i((padmask & NES_PAD_B) ? pad_active : pad_inactive);
    sdtx_putc(0x87); // btn
    sdtx_color1i((padmask & NES_PAD_A) ? pad_active : pad_inactive);
    sdtx_putc(0x87); // btn
    sdtx_font(0);

    // cartridge inserted LED
    sdtx_color1i(text_color);
    sdtx_puts(" CART: ");
    sdtx_color1i(nes_cartridge_inserted(&state.nes) ? cart_active : cart_inactive);
    sdtx_putc(0xCF);    // filled circle

    sdtx_font(0);
    sdtx_color1i(text_color);
    sdtx_pos(0.0f, 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", (float)state.frame_time_us * 0.001f, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = 120;
    if (fs_success(FS_SLOT_IMAGE) && clock_frame_count_60hz() > load_delay_frames) {

        bool load_success = false;
        if (fs_ext(FS_SLOT_IMAGE, "nes")) {
            load_success = nes_insert_cart(&state.nes, fs_data(FS_SLOT_IMAGE));
        }
        if (load_success) {
            if (clock_frame_count_60hz() > (load_delay_frames + 10)) {
                gfx_flash_success();
            }
        }
        else {
            gfx_flash_error();
        }
        fs_reset(FS_SLOT_IMAGE);
    }
}

void app_input(const sapp_event* event) {
    // accept dropped files also when ImGui grabs input
    if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
        fs_start_load_dropped_file(FS_SLOT_IMAGE);
    }
#ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        // input was handled by UI
        return;
    }
#endif
    switch (event->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP: {
            int c;
            switch (event->key_code) {
                case SAPP_KEYCODE_LEFT:         c = 0x01; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x02; break;
                case SAPP_KEYCODE_DOWN:         c = 0x03; break;
                case SAPP_KEYCODE_UP:           c = 0x04; break;
                case SAPP_KEYCODE_ENTER:        c = 0x05; break;
                case SAPP_KEYCODE_F:            c = 0x06; break;
                case SAPP_KEYCODE_D:            c = 0x07; break;
                case SAPP_KEYCODE_S:            c = 0x08; break;
                default:                        c = 0x00; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    nes_key_down(&state.nes, c);
                }
                else {
                    nes_key_up(&state.nes, c);
                }
            }
            break;
        default:
            break;
        }
    }
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(void) {
    ui_nes_draw(&state.ui);
}
#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .event_cb = app_input,
        .frame_cb = app_frame,
        .cleanup_cb = app_cleanup,
        .width = 800,
        .height = 600,
        .window_title = "NES",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
        .logger.func = slog_func,
    };
}
