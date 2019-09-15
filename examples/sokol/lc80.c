/*
    lc80.c
*/
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_args.h"
#include "sokol_audio.h"
#include "clock.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "systems/lc80.h"
#include "lc80-roms.h"

/* imports from lc80-ui.cc */
#include "ui.h"
void lc80ui_init(lc80_t* lc80);
void lc80ui_discard(void);
void lc80ui_draw(void);
void lc80ui_exec(uint32_t frame_time_us);

static lc80_t sys;

/* sokol-app entry, configure application callbacks and window */
static void app_init(void);
static void app_frame(void);
static void app_input(const sapp_event*);
static void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc) { .argc = argc, .argv = argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 1024,
        .height = 720,
        .window_title = "LC-80",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

lc80_desc_t lc80_desc(void) {
    return (lc80_desc_t) {
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .rom_ptr = dump_lc80_2k_bin,
        .rom_size = sizeof(dump_lc80_2k_bin)
    };
}

void app_init(void) {
    sg_setup(&(sg_desc){
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view
    });
    clock_init();
    saudio_setup(&(saudio_desc){0});

    lc80_desc_t desc = lc80_desc();
    lc80_init(&sys, &desc);
    lc80ui_init(&sys);
}

void app_frame(void) {
    lc80ui_exec(clock_frame_time());
    sg_begin_default_pass(&(sg_pass_action){0}, sapp_width(), sapp_height());
    ui_draw();
    sg_end_pass();
    sg_commit();
}

void app_input(const sapp_event* event) {
    if (ui_input(event)) {
        return;
    }
    else {
        // FIXME: forward input to emulator
    }
}

void app_cleanup(void) {
    lc80_discard(&sys);
    lc80ui_discard();
    saudio_shutdown();
    sg_shutdown();
    sargs_shutdown();
}
