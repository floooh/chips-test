#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_debugtext.h"
#include "sokol_gl.h"
#include "sokol_audio.h"
#include "sokol_log.h"
#include "sokol_imgui.h"
#include "sokol_glue.h"
#include "chips/chips_common.h"
#include "gfx.h"
#include "shaders.glsl.h"
#include <assert.h>
#include <stdlib.h> // malloc/free
#include <string.h>

#define GFX_DEF(v,def) (v?v:def)

typedef struct {
    bool valid;
    bool disable_speaker_icon;
    gfx_border_t border;
    struct {
        sg_image img;       // framebuffer texture, RGBA8 or R8 if paletted
        sg_image pal_img;   // optional color palette texture
        sg_sampler smp;
        chips_dim_t dim;
        bool paletted;
    } fb;
    struct {
        chips_rect_t view;
        chips_dim_t pixel_aspect;
        sg_image img;
        sg_sampler smp;
        sg_buffer vbuf;
        sg_pipeline pip;
        sg_attachments attachments;
        sg_pass_action pass_action;
    } offscreen;
    struct {
        sg_buffer vbuf;
        sg_pipeline pip;
        sg_pass_action pass_action;
        bool portrait;
    } display;
    struct {
        sg_image img;
        sg_sampler smp;
        sgl_pipeline pip;
        chips_dim_t dim;
    } icon;
    int flash_success_count;
    int flash_error_count;
    gfx_draw_extra_t draw_extra_cb;
} gfx_state_t;
static gfx_state_t state;

static const float gfx_verts[] = {
    0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f
};
static const float gfx_verts_rot[] = {
    0.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f
};
static const float gfx_verts_flipped[] = {
    0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 0.0f
};
static const float gfx_verts_flipped_rot[] = {
    0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 0.0f
};

static sg_range gfx_select_vertices(void) {
    return (sg_range){
        .ptr = sg_query_features().origin_top_left ?
               (state.display.portrait ? gfx_verts_rot : gfx_verts) :
               (state.display.portrait ? gfx_verts_flipped_rot : gfx_verts_flipped),
        .size = sizeof(gfx_verts),
    };
}

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
    state.disable_speaker_icon = true;
}

chips_dim_t gfx_pixel_aspect(void) {
    assert(state.valid);
    return state.offscreen.pixel_aspect;
}

sg_image gfx_create_icon_texture(const uint8_t* packed_pixels, int width, int height, int stride) {
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
        .data.subimage[0][0] = { .ptr=pixels, .size=pixel_data_size }
    });
    free(pixels);
    return img;
}

// this function will be called at init time and when the emulator framebuffer size changes
static void gfx_init_images_and_pass(void) {
    // destroy previous resources (if exist)
    sg_destroy_image(state.fb.img);
    sg_destroy_sampler(state.fb.smp);
    sg_destroy_image(state.offscreen.img);
    sg_destroy_sampler(state.offscreen.smp);
    sg_destroy_attachments(state.offscreen.attachments);

    // a texture with the emulator's raw pixel data
    assert((state.fb.dim.width > 0) && (state.fb.dim.height > 0));
    state.fb.img = sg_make_image(&(sg_image_desc){
        .width = state.fb.dim.width,
        .height = state.fb.dim.height,
        .pixel_format = state.fb.paletted ? SG_PIXELFORMAT_R8 : SG_PIXELFORMAT_RGBA8,
        .usage = SG_USAGE_STREAM,
    });

    // a sampler for sampling the emulators raw pixel data
    state.fb.smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });

    // 2x-upscaling render target texture, sampler and pass
    assert((state.offscreen.view.width > 0) && (state.offscreen.view.height > 0));
    state.offscreen.img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 2 * state.offscreen.view.width,
        .height = 2 * state.offscreen.view.height,
        .sample_count = 1,
    });
    state.offscreen.smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    });
    state.offscreen.attachments = sg_make_attachments(&(sg_attachments_desc){
        .colors[0].image = state.offscreen.img
    });
}

void gfx_init(const gfx_desc_t* desc) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 32,
        .image_pool_size = 128,
        .shader_pool_size = 16,
        .pipeline_pool_size = 16,
        .attachments_pool_size = 2,
        .environment = sglue_environment(),
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
    state.disable_speaker_icon = desc->disable_speaker_icon;
    state.border = desc->border;
    state.display.portrait = desc->display_info.portrait;
    state.draw_extra_cb = desc->draw_extra_cb;
    state.fb.dim =  desc->display_info.frame.dim;
    state.fb.paletted = 0 != desc->display_info.palette.ptr;
    state.offscreen.pixel_aspect.width = GFX_DEF(desc->pixel_aspect.width, 1);
    state.offscreen.pixel_aspect.height = GFX_DEF(desc->pixel_aspect.height, 1);
    state.offscreen.view = desc->display_info.screen;

    if (state.fb.paletted) {
        static uint32_t palette_buf[256];
        assert((desc->display_info.palette.size > 0) && (desc->display_info.palette.size <= sizeof(palette_buf)));
        memcpy(palette_buf, desc->display_info.palette.ptr, desc->display_info.palette.size);
        state.fb.pal_img = sg_make_image(&(sg_image_desc){
            .width = 256,
            .height = 1,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .data = {
                .subimage[0][0] = {
                    .ptr = palette_buf,
                    .size = sizeof(palette_buf)
                }
            }
        });
    }

    state.offscreen.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action = SG_LOADACTION_DONTCARE }
    };
    state.offscreen.vbuf = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(gfx_verts)
    });

    sg_shader shd = {0};
    if (state.fb.paletted) {
        shd = sg_make_shader(offscreen_pal_shader_desc(sg_query_backend()));
    }
    else {
        shd = sg_make_shader(offscreen_shader_desc(sg_query_backend()));
    }
    state.offscreen.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .depth.pixel_format = SG_PIXELFORMAT_NONE
    });

    state.display.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.05f, 0.05f, 0.05f, 1.0f } }
    };
    state.display.vbuf = sg_make_buffer(&(sg_buffer_desc){
        .data = gfx_select_vertices(),
    });

    state.display.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(display_shader_desc(sg_query_backend())),
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP
    });

    // create an unpacked speaker icon image and sokol-gl pipeline
    {
        // textures must be 2^n for WebGL
        state.icon.dim.width = speaker_icon.width;
        state.icon.dim.height = speaker_icon.height;
        state.icon.img = gfx_create_icon_texture(
            speaker_icon.pixels,
            speaker_icon.width,
            speaker_icon.height,
            speaker_icon.stride);
        state.icon.smp = sg_make_sampler(&(sg_sampler_desc){
            .min_filter = SG_FILTER_LINEAR,
            .mag_filter = SG_FILTER_LINEAR,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        });

        // sokol-gl pipeline for alpha-blended rendering
        state.icon.pip = sgl_make_pipeline(&(sg_pipeline_desc){
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

    // create image and pass resources
    gfx_init_images_and_pass();
}

/* apply a viewport rectangle to preserve the emulator's aspect ratio,
   and for 'portrait' orientations, keep the emulator display at the
   top, to make room at the bottom for mobile virtual keyboard
*/
static void apply_viewport(chips_dim_t canvas, chips_rect_t view, chips_dim_t pixel_aspect, gfx_border_t border) {
    float cw = (float) (canvas.width - border.left - border.right);
    if (cw < 1.0f) {
        cw = 1.0f;
    }
    float ch = (float) (canvas.height - border.top - border.bottom);
    if (ch < 1.0f) {
        ch = 1.0f;
    }
    const float canvas_aspect = cw / ch;
    const chips_dim_t aspect = pixel_aspect;
    const float emu_aspect = (float)(view.width * aspect.width) / (float)(view.height * aspect.height);
    float vp_x, vp_y, vp_w, vp_h;
    if (emu_aspect < canvas_aspect) {
        vp_y = (float)border.top;
        vp_h = ch;
        vp_w = (ch * emu_aspect);
        vp_x = border.left + (cw - vp_w) * 0.5f;
    }
    else {
        vp_x = (float)border.left;
        vp_w = cw;
        vp_h = (cw / emu_aspect);
        vp_y = border.top + (ch - vp_h) * 0.5f;
    }
    sg_apply_viewportf(vp_x, vp_y, vp_w, vp_h, true);
}

void gfx_draw(chips_display_info_t display_info) {
    assert(state.valid);
    assert((display_info.frame.dim.width > 0) && (display_info.frame.dim.height > 0));
    assert(display_info.frame.buffer.ptr && (display_info.frame.buffer.size > 0));
    assert((display_info.screen.width > 0) && (display_info.screen.height > 0));
    const chips_dim_t display = { .width = sapp_width(), .height = sapp_height() };

    state.offscreen.view = display_info.screen;

    // check if emulator framebuffer size has changed, need to create new backing texture
    if ((display_info.frame.dim.width != state.fb.dim.width) || (display_info.frame.dim.height != state.fb.dim.height)) {
        state.fb.dim = display_info.frame.dim;
        gfx_init_images_and_pass();
    }

    // if audio is off, draw speaker icon via sokol-gl
    if (!state.disable_speaker_icon && saudio_isvalid() && saudio_suspended()) {
        const float x0 = display.width - (float)state.icon.dim.width - 10.0f;
        const float x1 = x0 + (float)state.icon.dim.width;
        const float y0 = 10.0f;
        const float y1 = y0 + (float)state.icon.dim.height;
        const float alpha = (sapp_frame_count() & 0x20) ? 0.0f : 1.0f;
        sgl_defaults();
        sgl_enable_texture();
        sgl_texture(state.icon.img, state.icon.smp);
        sgl_matrix_mode_projection();
        sgl_ortho(0.0f, (float)display.width, (float)display.height, 0.0f, -1.0f, +1.0f);
        sgl_c4f(1.0f, 1.0f, 1.0f, alpha);
        sgl_load_pipeline(state.icon.pip);
        sgl_begin_quads();
        sgl_v2f_t2f(x0, y0, 0.0f, 0.0f);
        sgl_v2f_t2f(x1, y0, 1.0f, 0.0f);
        sgl_v2f_t2f(x1, y1, 1.0f, 1.0f);
        sgl_v2f_t2f(x0, y1, 0.0f, 1.0f);
        sgl_end();
    }

    // copy emulator pixel data into emulator framebuffer texture
    sg_update_image(state.fb.img, &(sg_image_data){
        .subimage[0][0] = {
            .ptr = display_info.frame.buffer.ptr,
            .size = display_info.frame.buffer.size,
        }
    });

    // upscale the original framebuffer 2x with nearest filtering
    sg_begin_pass(&(sg_pass){
        .action = state.offscreen.pass_action,
        .attachments = state.offscreen.attachments
    });
    sg_apply_pipeline(state.offscreen.pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = state.offscreen.vbuf,
        .images = {
            [IMG_fb_tex] = state.fb.img,
            [IMG_pal_tex] = state.fb.pal_img,
        },
        .samplers[SMP_smp] = state.fb.smp,
    });
    const offscreen_vs_params_t vs_params = {
        .uv_offset = {
            (float)state.offscreen.view.x / (float)state.fb.dim.width,
            (float)state.offscreen.view.y / (float)state.fb.dim.height,
        },
        .uv_scale = {
            (float)state.offscreen.view.width / (float)state.fb.dim.width,
            (float)state.offscreen.view.height / (float)state.fb.dim.height
        }
    };
    sg_apply_uniforms(UB_offscreen_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 4, 1);
    sg_end_pass();

    // tint the clear color red or green if flash feedback is requested
    if (state.flash_error_count > 0) {
        state.flash_error_count--;
        state.display.pass_action.colors[0].clear_value.r = 0.7f;
    }
    else if (state.flash_success_count > 0) {
        state.flash_success_count--;
        state.display.pass_action.colors[0].clear_value.g = 0.7f;
    }
    else {
        state.display.pass_action.colors[0].clear_value.r = 0.05f;
        state.display.pass_action.colors[0].clear_value.g = 0.05f;
    }

    // draw the final pass with linear filtering
    sg_begin_pass(&(sg_pass){
        .action = state.display.pass_action,
        .swapchain = sglue_swapchain()
    });
    apply_viewport(display, display_info.screen, state.offscreen.pixel_aspect, state.border);
    sg_apply_pipeline(state.display.pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = state.display.vbuf,
        .images[IMG_tex] = state.offscreen.img,
        .samplers[SMP_smp] = state.offscreen.smp,
    });
    sg_draw(0, 4, 1);
    sg_apply_viewport(0, 0, display.width, display.height, true);
    sdtx_draw();
    sgl_draw();
    if (state.draw_extra_cb) {
        state.draw_extra_cb(&(gfx_draw_info_t){
            .display_image = state.offscreen.img,
            .display_sampler = state.offscreen.smp,
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
    sg_shutdown();
}
