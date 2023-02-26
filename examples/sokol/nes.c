/*
    nes.c

    NES.
*/
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "common.h"
#include "chips/clk.h"
#include "chips/m6502.h"
#include "chips/r2c02.h"
#include "systems/nes.h"

static struct {
  nes_t nes;
  uint32_t frame_time_us;
  uint32_t ticks;
  double emu_time_ms;
} state;

static void draw_status_bar(void);

static void app_init(void) {
  nes_init(&state.nes);
  gfx_init(&(gfx_desc_t){
      .display_info = nes_display_info(&state.nes),
  });
  clock_init();
  prof_init();
}

static void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t emu_start_time = stm_now();
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(nes_display_info(&state.nes));
}

static void app_cleanup(void) {
  nes_discard(&state.nes);
  gfx_shutdown();
  sargs_shutdown();
}

static void draw_status_bar(void) {
    prof_push(PROF_EMU, (float)state.emu_time_ms);
    prof_stats_t emu_stats = prof_stats(PROF_EMU);

    const uint32_t text_color = 0xFFFFFFFF;

    const float w = sapp_widthf();
    const float h = sapp_heightf();
    sdtx_canvas(w, h);
    sdtx_origin(1.0f, (h / 8.0f) - 3.5f);

    sdtx_font(0);
    sdtx_color1i(text_color);
    sdtx_pos(0.0f, 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", (float)state.frame_time_us * 0.001f, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    const chips_display_info_t info = nes_display_info(0);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .cleanup_cb = app_cleanup,
        .width = info.screen.width,
        .height = info.screen.height,
        .window_title = "NES",
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
}
