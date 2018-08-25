/*
    kc85.c

    KC85/2, /3 and /4.
*/
#include "common/common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/kc85.h"
#include "roms/kc85-roms.h"

kc85_t kc85;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    args_init(argc, argv);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * KC85_DISPLAY_WIDTH,
        .height = 2 * KC85_DISPLAY_HEIGHT,
        .window_title = "KC85",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

void app_init(void) {
    gfx_init(KC85_DISPLAY_WIDTH, KC85_DISPLAY_HEIGHT, 1, 1);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    if (args_has("file")) {
        fs_load_file(args_string("file"));
    }
    kc85_type_t type = KC85_TYPE_2;
    if (args_has("type")) {
        if (args_string_compare("type", "kc85_3")) {
            type = KC85_TYPE_3;
        }
        else if (args_string_compare("type", "kc85_4")) {
            type = KC85_TYPE_4;
        }
    }
    kc85_init(&kc85, &(kc85_desc_t){
        .type = type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .rom_caos22 = dump_caos22,
        .rom_caos22_size = sizeof(dump_caos22),
        .rom_caos31 = dump_caos31,
        .rom_caos31_size = sizeof(dump_caos31),
        .rom_caos42c = dump_caos42c,
        .rom_caos42c_size = sizeof(dump_caos42c),
        .rom_caos42e = dump_caos42e,
        .rom_caos42e_size = sizeof(dump_caos42e),
        .rom_kcbasic = dump_basic_c0,
        .rom_kcbasic_size = sizeof(dump_basic_c0)
    });
}

void app_frame(void) {
    kc85_exec(&kc85, clock_frame_time());
    gfx_draw();
    /* FIXME: tape loading */
}

void app_input(const sapp_event* event) {
    // FIXME!
}

void app_cleanup(void) {
    kc85_discard(&kc85);
    saudio_shutdown();
    gfx_shutdown();
}
