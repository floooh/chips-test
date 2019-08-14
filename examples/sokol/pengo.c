/*
    pengo.c

    Pengo arcade machine emulator.
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "pengo-roms.h"
#define NAMCO_PENGO
#include "systems/namco.h"

/* imports from pengo-ui.cc */
#ifdef CHIPS_USE_UI
#include "ui.h"
void pengoui_init(namco_t* sys);
void pengoui_discard(void);
void pengoui_draw(void);
void pengoui_exec(namco_t* sys, uint32_t frame_time_us);
static const int ui_extra_height = 16;
#else
static const int ui_extra_height = 0;
#endif

namco_t sys;

/* sokol-app entry, configure application callbacks and window */
static void app_init(void);
static void app_frame(void);
static void app_input(const sapp_event*);
static void app_cleanup(void);

/* sokol-app entry */
sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = (5 * namco_std_display_width()) / 2,
        .height = 3 * namco_std_display_height() + ui_extra_height,
        .window_title = "Pengo Arcade"
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* application init callback */
static void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .top_offset = ui_extra_height,
        .aspect_x = 2,
        .aspect_y = 3,
        .rot90 = true
    });
    clock_init();
    saudio_setup(&(saudio_desc){0});
    namco_init(&sys, &(namco_desc_t){
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_cpu_0000_0FFF = dump_ep5120_8, .rom_cpu_0000_0FFF_size = sizeof(dump_ep5120_8),
        .rom_cpu_1000_1FFF = dump_ep5121_7, .rom_cpu_1000_1FFF_size = sizeof(dump_ep5121_7),
        .rom_cpu_2000_2FFF = dump_ep5122_15, .rom_cpu_2000_2FFF_size = sizeof(dump_ep5122_15),
        .rom_cpu_3000_3FFF = dump_ep5123_14, .rom_cpu_3000_3FFF_size = sizeof(dump_ep5123_14),
        .rom_cpu_4000_4FFF = dump_ep5124_21, .rom_cpu_4000_4FFF_size = sizeof(dump_ep5124_21),
        .rom_cpu_5000_5FFF = dump_ep5125_20, .rom_cpu_5000_5FFF_size = sizeof(dump_ep5125_20),
        .rom_cpu_6000_6FFF = dump_ep5126_32, .rom_cpu_6000_6FFF_size = sizeof(dump_ep5126_32),
        .rom_cpu_7000_7FFF = dump_ep5127_31, .rom_cpu_7000_7FFF_size = sizeof(dump_ep5127_31),
        .rom_gfx_0000_1FFF = dump_ep1640_92, .rom_gfx_0000_1FFF_size = sizeof(dump_ep1640_92),
        .rom_gfx_2000_3FFF = dump_ep1695_105,.rom_gfx_2000_3FFF_size = sizeof(dump_ep1695_105),
        .rom_prom_0000_001F = dump_pr1633_78, .rom_prom_0000_001F_size = sizeof(dump_pr1633_78),
        .rom_prom_0020_041F = dump_pr1634_88, .rom_prom_0020_041F_size = sizeof(dump_pr1634_88),
        .rom_sound_0000_00FF = dump_pr1635_51, .rom_sound_0000_00FF_size = sizeof(dump_pr1635_51),
        .rom_sound_0100_01FF = dump_pr1636_70, .rom_sound_0100_01FF_size = sizeof(dump_pr1636_70)
    });
    #ifdef CHIPS_USE_UI
    pengoui_init(&sys);
    #endif
}

static void app_frame(void) {
    #if CHIPS_USE_UI
        pengoui_exec(&sys, clock_frame_time());
    #else
        namco_exec(&sys, clock_frame_time());
    #endif
    namco_decode_video(&sys);
    gfx_draw(namco_display_width(&sys), namco_display_height(&sys));
}

static void app_input(const sapp_event* event) {
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        /* input was handled by UI */
        return;
    }
    #endif
    switch (event->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            switch (event->key_code) {
                case SAPP_KEYCODE_RIGHT:    namco_input_set(&sys, NAMCO_INPUT_P1_RIGHT); break;
                case SAPP_KEYCODE_LEFT:     namco_input_set(&sys, NAMCO_INPUT_P1_LEFT); break;
                case SAPP_KEYCODE_UP:       namco_input_set(&sys, NAMCO_INPUT_P1_UP); break;
                case SAPP_KEYCODE_DOWN:     namco_input_set(&sys, NAMCO_INPUT_P1_DOWN); break;
                case SAPP_KEYCODE_1:        namco_input_set(&sys, NAMCO_INPUT_P1_COIN); break;
                case SAPP_KEYCODE_2:        namco_input_set(&sys, NAMCO_INPUT_P2_COIN); break;
                case SAPP_KEYCODE_SPACE:    namco_input_set(&sys, NAMCO_INPUT_P1_BUTTON); break;
                default:                    namco_input_set(&sys, NAMCO_INPUT_P1_START); break;
            }
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_RIGHT:    namco_input_clear(&sys, NAMCO_INPUT_P1_RIGHT); break;
                case SAPP_KEYCODE_LEFT:     namco_input_clear(&sys, NAMCO_INPUT_P1_LEFT); break;
                case SAPP_KEYCODE_UP:       namco_input_clear(&sys, NAMCO_INPUT_P1_UP); break;
                case SAPP_KEYCODE_DOWN:     namco_input_clear(&sys, NAMCO_INPUT_P1_DOWN); break;
                case SAPP_KEYCODE_1:        namco_input_clear(&sys, NAMCO_INPUT_P1_COIN); break;
                case SAPP_KEYCODE_2:        namco_input_clear(&sys, NAMCO_INPUT_P2_COIN); break;
                case SAPP_KEYCODE_SPACE:    namco_input_clear(&sys, NAMCO_INPUT_P1_BUTTON); break;
                default:                    namco_input_clear(&sys, NAMCO_INPUT_P1_START); break;
            }
            break;
        default:
            break;
    }
}

static void app_cleanup(void) {
    #ifdef CHIPS_USE_UI
    pengoui_discard();
    #endif
    namco_discard(&sys);
    saudio_shutdown();
    gfx_shutdown();
}

