/*
    bombjack.c

    Bomb Jack arcade machine emulation (MAME used as reference).
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "bombjack-roms.h"

// the actual emulator is here: https://github.com/floooh/chips/blob/master/systems/bombjack.h
#include "systems/bombjack.h"

/* imports from bombjack-ui.cc */
#ifdef CHIPS_USE_UI
#include "ui.h"
void bombjackui_init(bombjack_t* bj);
void bombjackui_discard(void);
void bombjackui_draw(void);
void bombjackui_exec(bombjack_t* bj, uint32_t frame_time_us);
static const int ui_extra_height = 16;
#else
static const int ui_extra_height = 0;
#endif

bombjack_t bj;

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
        .width = 3 * bombjack_std_display_width(),
        .height = 3 * bombjack_std_display_height() + ui_extra_height,
        .window_title = "Bomb Jack Arcade"
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
        .aspect_x = 4,
        .aspect_y = 5,
        .rot90 = true
    });
    clock_init();
    saudio_setup(&(saudio_desc){0});
    bombjack_init(&bj, &(bombjack_desc_t){
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_main_0000_1FFF = dump_09_j01b_bin,     .rom_main_0000_1FFF_size = sizeof(dump_09_j01b_bin),
        .rom_main_2000_3FFF = dump_10_l01b_bin,     .rom_main_2000_3FFF_size = sizeof(dump_10_l01b_bin),
        .rom_main_4000_5FFF = dump_11_m01b_bin,     .rom_main_4000_5FFF_size = sizeof(dump_11_m01b_bin),
        .rom_main_6000_7FFF = dump_12_n01b_bin,     .rom_main_6000_7FFF_size = sizeof(dump_12_n01b_bin),
        .rom_main_C000_DFFF = dump_13_1r,           .rom_main_C000_DFFF_size = sizeof(dump_13_1r),
        .rom_sound_0000_1FFF = dump_01_h03t_bin,    .rom_sound_0000_1FFF_size = sizeof(dump_01_h03t_bin),
        .rom_chars_0000_0FFF = dump_03_e08t_bin,    .rom_chars_0000_0FFF_size = sizeof(dump_03_e08t_bin),
        .rom_chars_1000_1FFF = dump_04_h08t_bin,    .rom_chars_1000_1FFF_size = sizeof(dump_04_h08t_bin),
        .rom_chars_2000_2FFF = dump_05_k08t_bin,    .rom_chars_2000_2FFF_size = sizeof(dump_05_k08t_bin),
        .rom_tiles_0000_1FFF = dump_06_l08t_bin,    .rom_tiles_0000_1FFF_size = sizeof(dump_06_l08t_bin),
        .rom_tiles_2000_3FFF = dump_07_n08t_bin,    .rom_tiles_2000_3FFF_size = sizeof(dump_07_n08t_bin),
        .rom_tiles_4000_5FFF = dump_08_r08t_bin,    .rom_tiles_4000_5FFF_size = sizeof(dump_08_r08t_bin),
        .rom_sprites_0000_1FFF = dump_16_m07b_bin,  .rom_sprites_0000_1FFF_size = sizeof(dump_16_m07b_bin),
        .rom_sprites_2000_3FFF = dump_15_l07b_bin,  .rom_sprites_2000_3FFF_size = sizeof(dump_15_l07b_bin),
        .rom_sprites_4000_5FFF = dump_14_j07b_bin,  .rom_sprites_4000_5FFF_size = sizeof(dump_14_j07b_bin),
        .rom_maps_0000_0FFF = dump_02_p04t_bin,     .rom_maps_0000_0FFF_size = sizeof(dump_02_p04t_bin)
    });
    #ifdef CHIPS_USE_UI
    bombjackui_init(&bj);
    #endif
}

/* per-frame callback */
static void app_frame(void) {
    #if CHIPS_USE_UI
        bombjackui_exec(&bj, clock_frame_time());
    #else
        bombjack_exec(&bj, clock_frame_time());
    #endif
    bombjack_decode_video(&bj);
    gfx_draw(bombjack_display_width(&bj), bombjack_display_height(&bj));
}

/* input handling */
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
                /* player 1 joystick */
                case SAPP_KEYCODE_RIGHT: bj.mainboard.p1 |= BOMBJACK_JOYSTICK_RIGHT; break;
                case SAPP_KEYCODE_LEFT:  bj.mainboard.p1 |= BOMBJACK_JOYSTICK_LEFT; break;
                case SAPP_KEYCODE_UP:    bj.mainboard.p1 |= BOMBJACK_JOYSTICK_UP; break;
                case SAPP_KEYCODE_DOWN:  bj.mainboard.p1 |= BOMBJACK_JOYSTICK_DOWN; break;
                case SAPP_KEYCODE_SPACE: bj.mainboard.p1 |= BOMBJACK_JOYSTICK_BUTTON; break;
                /* player 1 coin */
                case SAPP_KEYCODE_1:     bj.mainboard.sys |= BOMBJACK_SYS_P1_COIN; break;
                /* player 1 start (any other key */
                default:                 bj.mainboard.sys |= BOMBJACK_SYS_P1_START; break;
            }
            break;

        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                /* player 1 joystick */
                case SAPP_KEYCODE_RIGHT: bj.mainboard.p1 &= ~BOMBJACK_JOYSTICK_RIGHT; break;
                case SAPP_KEYCODE_LEFT:  bj.mainboard.p1 &= ~BOMBJACK_JOYSTICK_LEFT; break;
                case SAPP_KEYCODE_UP:    bj.mainboard.p1 &= ~BOMBJACK_JOYSTICK_UP; break;
                case SAPP_KEYCODE_DOWN:  bj.mainboard.p1 &= ~BOMBJACK_JOYSTICK_DOWN; break;
                case SAPP_KEYCODE_SPACE: bj.mainboard.p1 &= ~BOMBJACK_JOYSTICK_BUTTON; break;
                /* player 1 coin */
                case SAPP_KEYCODE_1:     bj.mainboard.sys &= ~BOMBJACK_SYS_P1_COIN; break;
                /* player 1 start (any other key */
                default:                 bj.mainboard.sys &= ~BOMBJACK_SYS_P1_START; break;
            }
            break;
        default:
            break;
    }
}

/* app shutdown */
static void app_cleanup(void) {
    #ifdef CHIPS_USE_UI
    bombjackui_discard();
    #endif
    bombjack_discard(&bj);
    saudio_shutdown();
    gfx_shutdown();
}
