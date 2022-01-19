//------------------------------------------------------------------------------
//  ui.cc
//------------------------------------------------------------------------------
#include "ui.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_time.h"
#include "imgui.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"

static ui_draw_t ui_draw_cb;

void ui_init(ui_draw_t draw_cb) {
    simgui_desc_t simgui_desc = { };
    simgui_setup(&simgui_desc);
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;
    ui_draw_cb = draw_cb;
}

void ui_discard(void) {
    simgui_shutdown();
}

void ui_draw(void) {
    simgui_new_frame({sapp_width(), sapp_height(), sapp_frame_duration(), sapp_dpi_scale() });
    if (ui_draw_cb) {
        ui_draw_cb();
    }
    simgui_render();
}

bool ui_input(const sapp_event* event) {
    return simgui_handle_event(event);
}
