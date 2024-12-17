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
#include "gfx.h"
#include <stdlib.h> // calloc

#define UI_DELETE_STACK_SIZE (32)

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

static struct {
    ui_draw_t draw_cb;
    sg_sampler nearest_sampler;
    sg_sampler linear_sampler;
    sg_image empty_snapshot_texture;
    struct {
        sg_image images[UI_DELETE_STACK_SIZE];
        size_t cur_slot;
    } delete_stack;
} state;

static const struct {
    int width;
    int height;
    int stride;
    uint8_t pixels[32];
} empty_snapshot_icon = {
    .width = 16,
    .height = 16,
    .stride = 2,
    .pixels = {
        0xFF,0xFF,
        0x03,0xC0,
        0x05,0xA0,
        0x09,0x90,
        0x11,0x88,
        0x21,0x84,
        0x41,0x82,
        0x81,0x81,
        0x81,0x81,
        0x41,0x82,
        0x21,0x84,
        0x11,0x88,
        0x09,0x90,
        0x05,0xA0,
        0x03,0xC0,
        0xFF,0xFF,
    }
};

static void commit_listener(void* user_data) {
    (void)user_data;
    // garbage collect images
    for (size_t i = 0; i < state.delete_stack.cur_slot; i++) {
        sg_destroy_image(state.delete_stack.images[i]);
    }
    state.delete_stack.cur_slot = 0;
}

void ui_init(ui_draw_t draw_cb) {
    simgui_desc_t simgui_desc = { };
    simgui_setup(&simgui_desc);
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;
    state.draw_cb = draw_cb;

    sg_add_commit_listener({ .func = commit_listener });

    state.nearest_sampler = sg_make_sampler({
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    });

    state.linear_sampler = sg_make_sampler({
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    });

    state.empty_snapshot_texture = gfx_create_icon_texture(
        empty_snapshot_icon.pixels,
        empty_snapshot_icon.width,
        empty_snapshot_icon.height,
        empty_snapshot_icon.stride);
}

void ui_discard(void) {
    sg_destroy_sampler(state.nearest_sampler);
    sg_destroy_sampler(state.linear_sampler);
    sg_remove_commit_listener({ .func = commit_listener });
    simgui_shutdown();
}

void ui_draw(void) {
    simgui_new_frame({sapp_width(), sapp_height(), sapp_frame_duration(), sapp_dpi_scale() });
    if (state.draw_cb) {
        state.draw_cb();
    }
    simgui_render();
}

bool ui_input(const sapp_event* event) {
    return simgui_handle_event(event);
}

ui_texture_t ui_create_texture(int w, int h) {
    return simgui_imtextureid_with_sampler(
        sg_make_image({
            .width = w,
            .height = h,
            .usage = SG_USAGE_STREAM,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
        }),
        state.nearest_sampler);
}

void ui_update_texture(ui_texture_t h, void* data, int data_byte_size) {
    sg_image img = simgui_image_from_imtextureid(h);
    sg_image_data img_data = { };
    img_data.subimage[0][0] = { .ptr = data, .size = (size_t) data_byte_size };
    sg_update_image(img, img_data);
}

void ui_destroy_texture(ui_texture_t h) {
    if (state.delete_stack.cur_slot < UI_DELETE_STACK_SIZE) {
        state.delete_stack.images[state.delete_stack.cur_slot++] = simgui_image_from_imtextureid(h);
    }
}

// creates a 2x downscaled screenshot texture of the emulator framebuffer
ui_texture_t ui_create_screenshot_texture(chips_display_info_t info) {
    assert(info.frame.buffer.ptr);

    size_t dst_w = (info.screen.width + 1) >> 1;
    size_t dst_h = (info.screen.height + 1) >> 1;
    size_t dst_num_bytes = (size_t)(dst_w * dst_h * 4);
    uint32_t* dst = (uint32_t*) calloc(1, dst_num_bytes);

    if (info.palette.ptr) {
        assert(info.frame.bytes_per_pixel == 1);
        const uint8_t* pixels = (uint8_t*) info.frame.buffer.ptr;
        const uint32_t* palette = (uint32_t*) info.palette.ptr;
        const size_t num_palette_entries = info.palette.size / sizeof(uint32_t);
        for (size_t y = 0; y < (size_t)info.screen.height; y++) {
            for (size_t x = 0; x < (size_t)info.screen.width; x++) {
                uint8_t p = pixels[(y + info.screen.y) * info.frame.dim.width + (x + info.screen.x)];
                assert(p < num_palette_entries); (void)num_palette_entries;
                uint32_t c = (palette[p] >> 2) & 0x3F3F3F3F;
                size_t dst_x = x >> 1;
                size_t dst_y = y >> 1;
                if (info.portrait) {
                    dst[dst_x * dst_h + (dst_h - dst_y - 1)] += c;
                }
                else {
                    dst[dst_y * dst_w + dst_x] += c;
                }
            }
        }
    }
    else {
        assert(info.frame.bytes_per_pixel == 4);
        const uint32_t* pixels = (uint32_t*) info.frame.buffer.ptr;
        for (size_t y = 0; y < (size_t)info.screen.height; y++) {
            for (size_t x = 0; x < (size_t)info.screen.width; x++) {
                uint32_t c = pixels[(y + info.screen.y) * info.frame.dim.width + (x + info.screen.x)];
                c = (c >> 2) & 0x3F3F3F3F;
                size_t dst_x = x >> 1;
                size_t dst_y = y >> 1;
                if (info.portrait) {
                    dst[dst_x * dst_h + (dst_h - dst_y - 1)] += c;
                }
                else {
                    dst[dst_y * dst_w + dst_x] += c;
                }
            }
        }
    }

    sg_image_desc img_desc = {
        .width = (int) (info.portrait ? dst_h : dst_w),
        .height = (int) (info.portrait ? dst_w : dst_h),
        .pixel_format = SG_PIXELFORMAT_RGBA8,
    };
    img_desc.data.subimage[0][0] = { .ptr = dst, .size = dst_num_bytes };
    sg_image img = sg_make_image(img_desc);
    free(dst);
    return simgui_imtextureid_with_sampler(img, state.linear_sampler);
}

ui_texture_t ui_shared_empty_snapshot_texture(void) {
    return simgui_imtextureid_with_sampler(state.empty_snapshot_texture, state.nearest_sampler);
}
