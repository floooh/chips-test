#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_debugtext.h"
#include "sokol_gl.h"
#include "sokol_audio.h"
#include "sokol_log.h"
#include "sokol_imgui.h"
#include "sokol_letterbox.h"
#include "sokol_framebuffer.h"
#include "sokol_glue.h"
#include "chips/chips_common.h"
#include "gfx.h"
#include <assert.h>
#include <stdlib.h> // malloc/free
#include <string.h>

#define GFX_DEF(v,def) (v?v:def)

typedef struct {
    bool valid;
    gfx_border_t border;
    chips_dim_t pixel_aspect;
    bool paletted;
    sfb_framebuffer fb;
    sg_pass_action pass_action;
    struct {
        bool disable;
        sg_view tex_view;
        sg_sampler smp;
        sgl_pipeline pip;
        chips_dim_t dim;
    } speaker_icon;
    int flash_success_count;
    int flash_error_count;
    gfx_draw_extra_t draw_extra_cb;
    uint32_t palette[256];
} gfx_state_t;
static gfx_state_t state;

// a bit-packed speaker-off icon
static const struct {
    int width;
    int height;
    int stride;
    uint8_t pixels[350];
} speaker_icon = {
    .width = 50,
    .height = 50,
    .stride = 7,
    .pixels = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x80,0x00,0x00,0x00,0x00,0x00,0x00,
        0xC0,0x01,0x00,0x00,0x00,0x00,0x00,
        0xE0,0x03,0x40,0x00,0x00,0x00,0x00,
        0xF0,0x07,0x60,0x00,0x80,0x03,0x00,
        0xE0,0x0F,0x70,0x00,0xC0,0x07,0x00,
        0xC0,0x1F,0x78,0x00,0xE0,0x07,0x00,
        0x80,0x3F,0x78,0x00,0xE0,0x0F,0x00,
        0x00,0x7F,0x78,0x00,0xC0,0x1F,0x00,
        0x00,0xFE,0x70,0x00,0x80,0x1F,0x00,
        0x00,0xFC,0x61,0x00,0x8E,0x3F,0x00,
        0x00,0xF8,0x43,0x00,0x1F,0x3F,0x00,
        0x00,0xF0,0x07,0x80,0x1F,0x7E,0x00,
        0x00,0xF0,0x0F,0x80,0x3F,0x7E,0x00,
        0x00,0xF8,0x1F,0x00,0x3F,0x7C,0x00,
        0xF0,0xFF,0x3F,0x00,0x7E,0xFC,0x00,
        0xF8,0xFF,0x7F,0x00,0x7E,0xFC,0x00,
        0xFC,0x7F,0xFE,0x00,0xFC,0xF8,0x00,
        0xFC,0x3F,0xFC,0x01,0xFC,0xF8,0x00,
        0xFC,0x1F,0xFC,0x03,0xF8,0xF8,0x00,
        0x7C,0x00,0xFC,0x07,0xF8,0xF8,0x00,
        0x7C,0x00,0xFC,0x0F,0xF8,0xF8,0x00,
        0x7C,0x00,0xFC,0x1F,0xF8,0xF8,0x00,
        0x7C,0x00,0xFC,0x3F,0xF8,0xF8,0x00,
        0xFC,0x1F,0x7C,0x7F,0xF8,0xF8,0x00,
        0xFC,0x3F,0x7C,0xFE,0xF0,0xF8,0x00,
        0xFC,0x7F,0x7C,0xFC,0xE1,0xF8,0x00,
        0xF8,0xFF,0x7C,0xF8,0x03,0xFC,0x00,
        0xE0,0xFF,0x7D,0xF0,0x07,0xFC,0x00,
        0x00,0xF8,0x7F,0xE0,0x0F,0x7C,0x00,
        0x00,0xF0,0x7F,0xC0,0x1F,0x7C,0x00,
        0x00,0xE0,0x7F,0x80,0x3F,0x7C,0x00,
        0x00,0xC0,0x7F,0x00,0x7F,0x38,0x00,
        0x00,0x80,0x7F,0x00,0xFE,0x30,0x00,
        0x00,0x00,0x7F,0x00,0xFC,0x01,0x00,
        0x00,0x00,0x7E,0x00,0xF8,0x03,0x00,
        0x00,0x00,0x7C,0x00,0xF0,0x07,0x00,
        0x00,0x00,0x78,0x00,0xE0,0x0F,0x00,
        0x00,0x00,0x70,0x00,0xC0,0x1F,0x00,
        0x00,0x00,0x60,0x00,0x80,0x3F,0x00,
        0x00,0x00,0x40,0x00,0x00,0x1F,0x00,
        0x00,0x00,0x00,0x00,0x00,0x0E,0x00,
        0x00,0x00,0x00,0x00,0x00,0x04,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    }
};

void gfx_flash_success(void) {
    assert(state.valid);
    state.flash_success_count = 20;
}

void gfx_flash_error(void) {
    assert(state.valid);
    state.flash_error_count = 20;
}

void gfx_disable_speaker_icon(void) {
    assert(state.valid);
    state.speaker_icon.disable = true;
}

chips_dim_t gfx_pixel_aspect(void) {
    assert(state.valid);
    return state.pixel_aspect;
}

sg_view gfx_create_icon_texview(const uint8_t* packed_pixels, int width, int height, int stride, const char* label) {
    const size_t pixel_data_size = width * height * sizeof(uint32_t);
    uint32_t* pixels = malloc(pixel_data_size);
    assert(pixels);
    memset(pixels, 0, pixel_data_size);
    const uint8_t* src = packed_pixels;
    uint32_t* dst = pixels;
    for (int y = 0; y < height; y++) {
        uint8_t bits = 0;
        dst = pixels + (y * width);
        for (int x = 0; x < width; x++) {
            if ((x & 7) == 0) {
                bits = *src++;
            }
            if (bits & 1) {
                *dst++ = 0xFFFFFFFF;
            }
            else {
                *dst++ = 0x00FFFFFF;
            }
            bits >>= 1;
        }
    }
    assert(src == packed_pixels + stride * height); (void)stride;   // stride is unused in release mode
    assert(dst <= pixels + (width * height));
    sg_image img = sg_make_image(&(sg_image_desc){
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .width = width,
        .height = height,
        .data.mip_levels[0] = { .ptr=pixels, .size=pixel_data_size },
        .label = label,
    });
    free(pixels);
    sg_view view = sg_make_view(&(sg_view_desc){ .texture.image = img, .label = label });
    return view;
}

void gfx_init(const gfx_desc_t* desc) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 32,
        .image_pool_size = 128,
        .shader_pool_size = 16,
        .pipeline_pool_size = 16,
        .view_pool_size = 256,
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
    if (desc->init_extra_cb) {
        desc->init_extra_cb();
    }
    sfb_setup(&(sfb_desc){
        .framebuffer_pool_size = 1,
        .logger.func = slog_func,
    });
    sgl_setup(&(sgl_desc_t){
        .max_vertices = 16,
        .max_commands = 16,
        .context_pool_size = 1,
        .pipeline_pool_size = 16,
        .logger.func = slog_func,
    });
    sdtx_setup(&(sdtx_desc_t){
        .context_pool_size = 1,
        .fonts[0] = sdtx_font_z1013(),
        .fonts[1] = sdtx_font_kc853(),
        .logger.func = slog_func,
    });

    state.valid = true;
    state.speaker_icon.disable = desc->disable_speaker_icon;
    state.border = desc->border;
    state.pixel_aspect.width = GFX_DEF(desc->pixel_aspect.width, 1);
    state.pixel_aspect.height = GFX_DEF(desc->pixel_aspect.height, 1);
    state.paletted = desc->display_info.palette.ptr != 0;
    state.draw_extra_cb = desc->draw_extra_cb;
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.05f, 0.05f, 0.05f, 1.0f } }
    };

    state.fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = desc->display_info.frame.dim.width,
        .height = desc->display_info.frame.dim.height,
        .format = state.paletted ? SFB_FORMAT_PALETTE8 : SFB_FORMAT_RGBA8,
        .prescale = 2,
        .rotate90 = desc->display_info.portrait,
        .cliprect = {
            .x = desc->display_info.screen.x,
            .y = desc->display_info.screen.y,
            .width = desc->display_info.screen.width,
            .height = desc->display_info.screen.height,
        },
    });

    if (state.paletted) {
        assert(desc->display_info.palette.size <= sizeof(state.palette));
        memcpy(state.palette, desc->display_info.palette.ptr, desc->display_info.palette.size);
    }

    // create an unpacked speaker icon image and sokol-gl pipeline
    {
        // textures must be 2^n for WebGL
        state.speaker_icon.dim.width = speaker_icon.width;
        state.speaker_icon.dim.height = speaker_icon.height;
        state.speaker_icon.tex_view = gfx_create_icon_texview(
            speaker_icon.pixels,
            speaker_icon.width,
            speaker_icon.height,
            speaker_icon.stride,
            "speaker-icon"
        );
        state.speaker_icon.smp = sg_make_sampler(&(sg_sampler_desc){
            .min_filter = SG_FILTER_LINEAR,
            .mag_filter = SG_FILTER_LINEAR,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
            .label = "speaker-icon",
        });

        // sokol-gl pipeline for alpha-blended rendering
        state.speaker_icon.pip = sgl_make_pipeline(&(sg_pipeline_desc){
            .colors[0] = {
                .write_mask = SG_COLORMASK_RGB,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                }
            }
        });
    }
}

static void apply_viewport(chips_dim_t canvas, chips_rect_t view, chips_dim_t pixel_aspect, gfx_border_t border) {
    float emu_aspect = (float)(view.width * pixel_aspect.width) / (float)(view.height * pixel_aspect.height);
    const int cw = canvas.width;
    const int ch = canvas.height;
    const slbx_viewport vp = slbx_letterbox(cw, ch, &(slbx_letterbox_desc){
        .content_aspect_ratio = emu_aspect,
        .border = {
            .left = border.left,
            .right = border.right,
            .top = border.top,
            .bottom = border.bottom,
        }
    });
    sg_apply_viewport(vp.x, vp.y, vp.width, vp.height, true);
}

void gfx_draw(chips_display_info_t display_info) {
    assert(state.valid);
    assert((display_info.frame.dim.width > 0) && (display_info.frame.dim.height > 0));
    assert(display_info.frame.buffer.ptr && (display_info.frame.buffer.size > 0));
    assert((display_info.screen.width > 0) && (display_info.screen.height > 0));
    const chips_dim_t display = { .width = sapp_width(), .height = sapp_height() };

    // if audio is off, draw speaker icon via sokol-gl
    if (!state.speaker_icon.disable && saudio_isvalid() && saudio_suspended()) {
        const float x0 = display.width - (float)state.speaker_icon.dim.width - 10.0f;
        const float x1 = x0 + (float)state.speaker_icon.dim.width;
        const float y0 = 10.0f;
        const float y1 = y0 + (float)state.speaker_icon.dim.height;
        const float alpha = (sapp_frame_count() & 0x20) ? 0.0f : 1.0f;
        sgl_defaults();
        sgl_enable_texture();
        sgl_texture(state.speaker_icon.tex_view, state.speaker_icon.smp);
        sgl_matrix_mode_projection();
        sgl_ortho(0.0f, (float)display.width, (float)display.height, 0.0f, -1.0f, +1.0f);
        sgl_c4f(1.0f, 1.0f, 1.0f, alpha);
        sgl_load_pipeline(state.speaker_icon.pip);
        sgl_begin_quads();
        sgl_v2f_t2f(x0, y0, 0.0f, 0.0f);
        sgl_v2f_t2f(x1, y0, 1.0f, 0.0f);
        sgl_v2f_t2f(x1, y1, 1.0f, 1.0f);
        sgl_v2f_t2f(x0, y1, 0.0f, 1.0f);
        sgl_end();
    }

    // NOTE: resize is lazy, if nothing changed this call is a no-op
    sfb_resize(state.fb, &(sfb_resize_desc){
        .width = display_info.frame.dim.width,
        .height = display_info.frame.dim.height,
        .prescale = 2,
        .cliprect = {
            .x = display_info.screen.x,
            .y = display_info.screen.y,
            .width = display_info.screen.width,
            .height = display_info.screen.height,
        },
    });

    // update framebuffer pixel, palette and cliprect
    sfb_update(state.fb, &(sfb_update_desc){
        .pixels = {
            .ptr = display_info.frame.buffer.ptr,
            .size = display_info.frame.buffer.size,
        },
        .palette = {
            .ptr = state.palette,
            .size = sizeof(state.palette),
        },
    });

    // tint the clear color red or green if flash feedback is requested
    if (state.flash_error_count > 0) {
        state.flash_error_count--;
        state.pass_action.colors[0].clear_value.r = 0.7f;
    }
    else if (state.flash_success_count > 0) {
        state.flash_success_count--;
        state.pass_action.colors[0].clear_value.g = 0.7f;
    }
    else {
        state.pass_action.colors[0].clear_value.r = 0.05f;
        state.pass_action.colors[0].clear_value.g = 0.05f;
    }

    // draw the final pass with linear filtering
    sg_begin_pass(&(sg_pass){
        .action = state.pass_action,
        .swapchain = sglue_swapchain()
    });
    apply_viewport(display, display_info.screen, state.pixel_aspect, state.border);
    sfb_render(state.fb);
    sg_apply_viewport(0, 0, display.width, display.height, true);
    sdtx_draw();
    sgl_draw();
    if (state.draw_extra_cb) {
        const sfb_framebuffer_info info = sfb_query_framebuffer_info(state.fb);
        state.draw_extra_cb(&(gfx_draw_info_t){
            .display_texview = info.offscreen.tex_view,
            .display_sampler = info.nearest_sampler,
            .display_info = display_info,
        });
    }
    sg_end_pass();
    sg_commit();
}

void gfx_shutdown() {
    assert(state.valid);
    sgl_shutdown();
    sdtx_shutdown();
    sfb_shutdown();
    sg_shutdown();
}
