#pragma once
/*
    Common graphics functions for the chips-test example emulators.

    REMINDER: consider using this CRT shader?

    https://github.com/mattiasgustavsson/rebasic/blob/master/source/libs/crtemu.h
*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GFX_MAX_FB_WIDTH (1024)
#define GFX_MAX_FB_HEIGHT (1024)

typedef struct {
    int top, bottom, left, right;
} gfx_border_t;

typedef struct {
    int width, height;
} gfx_dim_t;

typedef struct {
    int x, y, width, height;
} gfx_rect_t;

typedef struct {
    gfx_border_t border;
    gfx_dim_t pixel_aspect;
    bool rot90;
    void (*draw_extra_cb)(void);
} gfx_desc_t;

typedef struct {
    // the framebuffer width and height
    gfx_dim_t fb;
    // the visible screen rectangle inside the framebuffer
    gfx_rect_t view;
} gfx_draw_t;

void gfx_init(const gfx_desc_t* desc);
uint32_t* gfx_framebuffer_ptr(void);
size_t gfx_framebuffer_size(void);
void gfx_draw(const gfx_draw_t* draw_args);
void gfx_shutdown(void);

// helper functions unrelated to actual emulator framebuffer rendering
void* gfx_create_texture(gfx_dim_t size);
void gfx_update_texture(void* h, void* data, int data_byte_size);
void gfx_destroy_texture(void* h);
void gfx_flash_success(void);
void gfx_flash_error(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*== IMPLEMENTATION ==========================================================*/
#ifdef COMMON_IMPL

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_debugtext.h"
#include "sokol_gl.h"
#include "sokol_audio.h"
#include "sokol_glue.h"
#include "shaders.glsl.h"
#include <assert.h>
#include <stdlib.h> // malloc/free

#define _GFX_DEF(v,def) (v?v:def)

typedef struct {
    bool valid;
    gfx_border_t border;
    struct {
        sg_image img;
        gfx_dim_t size;
    } fb;
    struct {
        gfx_rect_t view;
        gfx_dim_t pixel_aspect;
        sg_buffer vbuf;
        sg_pipeline pip;
        sg_image img;
        sg_pass pass;
        sg_pass_action pass_action;
    } offscreen;
    struct {
        sg_buffer vbuf;
        sg_pipeline pip;
        sg_pass_action pass_action;
        bool rot90;
    } display;
    struct {
        sg_image img;
        sgl_pipeline pip;
        gfx_dim_t size;
    } icon;
    int flash_success_count;
    int flash_error_count;

    uint32_t rgba8_buffer[GFX_MAX_FB_WIDTH * GFX_MAX_FB_HEIGHT];
    void (*draw_extra_cb)(void);
} gfx_state_t;

static gfx_state_t gfx;

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
    assert(gfx.valid);
    gfx.flash_success_count = 20;
}

void gfx_flash_error(void) {
    assert(gfx.valid);
    gfx.flash_error_count = 20;
}

uint32_t* gfx_framebuffer_ptr(void) {
    assert(gfx.valid);
    return gfx.rgba8_buffer;
}

size_t gfx_framebuffer_size(void) {
    assert(gfx.valid);
    return sizeof(gfx.rgba8_buffer);
}

static void gfx_init_images_and_pass(void) {
    // destroy previous resources (if exist)
    sg_destroy_image(gfx.fb.img);
    sg_destroy_image(gfx.offscreen.img);
    sg_destroy_pass(gfx.offscreen.pass);

    // a texture with the emulator's raw pixel data
    gfx.fb.img = sg_make_image(&(sg_image_desc){
        .width = gfx.fb.size.width,
        .height = gfx.fb.size.height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = SG_USAGE_STREAM,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });

    // 2x-upscaling render target textures and passes
    gfx.offscreen.img = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 2 * gfx.offscreen.view.width,
        .height = 2 * gfx.offscreen.view.height,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });
    gfx.offscreen.pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = gfx.offscreen.img
    });
}

void gfx_init(const gfx_desc_t* desc) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 32,
        .image_pool_size = 128,
        .shader_pool_size = 16,
        .pipeline_pool_size = 16,
        .context_pool_size = 2,
        .context = sapp_sgcontext()
    });
    sgl_setup(&(sgl_desc_t){
        .max_vertices = 16,
        .max_commands = 16,
        .context_pool_size = 1,
        .pipeline_pool_size = 16,
    });
    sdtx_setup(&(sdtx_desc_t){
        .context_pool_size = 1,
        .fonts[0] = sdtx_font_z1013(),
        .fonts[1] = sdtx_font_kc853()
    });

    gfx.valid = true;
    gfx.border = desc->border;
    gfx.fb.size =  (gfx_dim_t){0};

    gfx.offscreen.pixel_aspect.width = _GFX_DEF(desc->pixel_aspect.width, 1);
    gfx.offscreen.pixel_aspect.height = _GFX_DEF(desc->pixel_aspect.height, 1);

    gfx.offscreen.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_DONTCARE }
    };
    gfx.offscreen.vbuf = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(gfx_verts)
    });
    gfx.offscreen.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(offscreen_shader_desc(sg_query_backend())),
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .depth.pixel_format = SG_PIXELFORMAT_NONE
    });

    gfx.display.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.05f, 0.05f, 0.05f, 1.0f } }
    };
    gfx.display.vbuf = sg_make_buffer(&(sg_buffer_desc){
        .data = {
            .ptr = sg_query_features().origin_top_left ?
                   (desc->rot90 ? gfx_verts_rot : gfx_verts) :
                   (desc->rot90 ? gfx_verts_flipped_rot : gfx_verts_flipped),
            .size = sizeof(gfx_verts)
        }
    });
    gfx.display.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(display_shader_desc(sg_query_backend())),
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP
    });
    gfx.display.rot90 = desc->rot90;
    gfx.draw_extra_cb = desc->draw_extra_cb;

    // create an unpacked speaker icon image and sokol-gl pipeline
    {
        // textures must be 2^n for WebGL
        gfx.icon.size.width = speaker_icon.width;
        gfx.icon.size.height = speaker_icon.height;
        const size_t pixel_data_size = gfx.icon.size.width * gfx.icon.size.height * sizeof(uint32_t);
        uint32_t* pixels = malloc(pixel_data_size);
        assert(pixels);
        memset(pixels, 0, pixel_data_size);
        const uint8_t* src = speaker_icon.pixels;
        uint32_t* dst = pixels;
        for (int y = 0; y < gfx.icon.size.height; y++) {
            uint8_t bits = 0;
            dst = pixels + (y * gfx.icon.size.width);
            for (int x = 0; x < gfx.icon.size.width; x++) {
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
        assert(src == speaker_icon.pixels + speaker_icon.stride * speaker_icon.height);
        assert(dst <= pixels + (gfx.icon.size.width * gfx.icon.size.height));
        gfx.icon.img = sg_make_image(&(sg_image_desc){
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .width = gfx.icon.size.width,
            .height = gfx.icon.size.height,
            .min_filter = SG_FILTER_LINEAR,
            .mag_filter = SG_FILTER_LINEAR,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
            .data.subimage[0][0] = { .ptr=pixels, .size=pixel_data_size }
        });
        free(pixels);

        // sokol-gl pipeline for alpha-blended rendering
        gfx.icon.pip = sgl_make_pipeline(&(sg_pipeline_desc){
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

/* apply a viewport rectangle to preserve the emulator's aspect ratio,
   and for 'portrait' orientations, keep the emulator display at the
   top, to make room at the bottom for mobile virtual keyboard
*/
static void apply_viewport(gfx_dim_t canvas) {
    float cw = (float) (canvas.width - gfx.border.left - gfx.border.right);
    if (cw < 1.0f) {
        cw = 1.0f;
    }
    float ch = (float) (canvas.height - gfx.border.top - gfx.border.bottom);
    if (ch < 1.0f) {
        ch = 1.0f;
    }
    const float canvas_aspect = (float)cw / (float)ch;
    const gfx_rect_t view = gfx.offscreen.view;
    const gfx_dim_t aspect = gfx.offscreen.pixel_aspect;
    const float emu_aspect = (float)(view.width * aspect.width) / (float)(view.height * aspect.height);
    float vp_x, vp_y, vp_w, vp_h;
    if (emu_aspect < canvas_aspect) {
        vp_y = (float)gfx.border.top;
        vp_h = ch;
        vp_w = (ch * emu_aspect);
        vp_x = gfx.border.left + (cw - vp_w) / 2;
    }
    else {
        vp_x = (float)gfx.border.left;
        vp_w = cw;
        vp_h = (cw / emu_aspect);
        vp_y = (float)gfx.border.top;
    }
    sg_apply_viewportf(vp_x, vp_y, vp_w, vp_h, true);
}

void gfx_draw(const gfx_draw_t* args) {
    assert(gfx.valid);
    assert(args);
    assert((args->fb.width > 0) && (args->fb.height > 0));
    assert((args->view.width > 0) && (args->view.height > 0));
    const gfx_dim_t display = { .width = sapp_width(), .height = sapp_height() };

    gfx.offscreen.view = args->view;

    // check if emulator framebuffer size has changed, need to create new backing texture
    if ((args->fb.width != gfx.fb.size.width) || (args->fb.height != gfx.fb.size.height)) {
        gfx.fb.size = args->fb;
        gfx_init_images_and_pass();
    }

    // if audio is off, draw speaker icon via sokol-gl
    if (saudio_suspended()) {
        const float x0 = display.width - (float)gfx.icon.size.width - 10.0f;
        const float x1 = x0 + (float)gfx.icon.size.width;
        const float y0 = 10.0f;
        const float y1 = y0 + (float)gfx.icon.size.height;
        const float alpha = (sapp_frame_count() & 0x20) ? 0.0f : 1.0f;
        sgl_defaults();
        sgl_enable_texture();
        sgl_texture(gfx.icon.img);
        sgl_matrix_mode_projection();
        sgl_ortho(0.0f, (float)display.width, (float)display.height, 0.0f, -1.0f, +1.0f);
        sgl_c4f(1.0f, 1.0f, 1.0f, alpha);
        sgl_load_pipeline(gfx.icon.pip);
        sgl_begin_quads();
        sgl_v2f_t2f(x0, y0, 0.0f, 0.0f);
        sgl_v2f_t2f(x1, y0, 1.0f, 0.0f);
        sgl_v2f_t2f(x1, y1, 1.0f, 1.0f);
        sgl_v2f_t2f(x0, y1, 0.0f, 1.0f);
        sgl_end();
    }

    // copy emulator pixel data into emulator framebuffer texture
    sg_update_image(gfx.fb.img, &(sg_image_data){
        .subimage[0][0] = {
            .ptr = gfx.rgba8_buffer,
            .size = gfx.fb.size.width * gfx.fb.size.height * sizeof(uint32_t)
        }
    });

    // upscale the original framebuffer 2x with nearest filtering
    sg_begin_pass(gfx.offscreen.pass, &gfx.offscreen.pass_action);
    sg_apply_pipeline(gfx.offscreen.pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = gfx.offscreen.vbuf,
        .fs_images[SLOT_fb_tex] = gfx.fb.img,
    });
    const offscreen_vs_params_t vs_params = {
        .uv_offset = {
            (float)gfx.offscreen.view.x / (float)gfx.fb.size.width,
            (float)gfx.offscreen.view.y / (float)gfx.fb.size.height,
        },
        .uv_scale = {
            (float)gfx.offscreen.view.width / (float)gfx.fb.size.width,
            (float)gfx.offscreen.view.height / (float)gfx.fb.size.height
        }
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_offscreen_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 4, 1);
    sg_end_pass();

    // tint the clear color red or green if flash feedback is requested
    if (gfx.flash_error_count > 0) {
        gfx.flash_error_count--;
        gfx.display.pass_action.colors[0].value.r = 0.7f;
    }
    else if (gfx.flash_success_count > 0) {
        gfx.flash_success_count--;
        gfx.display.pass_action.colors[0].value.g = 0.7f;
    }
    else {
        gfx.display.pass_action.colors[0].value.r = 0.05f;
        gfx.display.pass_action.colors[0].value.g = 0.05f;
    }

    // draw the final pass with linear filtering
    sg_begin_default_pass(&gfx.display.pass_action, display.width, display.height);
    apply_viewport(display);
    sg_apply_pipeline(gfx.display.pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = gfx.display.vbuf,
        .fs_images[SLOT_tex] = gfx.offscreen.img,
    });
    sg_draw(0, 4, 1);
    sg_apply_viewport(0, 0, display.width, display.height, true);
    sdtx_draw();
    sgl_draw();
    if (gfx.draw_extra_cb) {
        gfx.draw_extra_cb();
    }
    sg_end_pass();
    sg_commit();
}

void gfx_shutdown() {
    assert(gfx.valid);
    sgl_shutdown();
    sdtx_shutdown();
    sg_shutdown();
}

void* gfx_create_texture(gfx_dim_t size) {
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = size.width,
        .height = size.height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = SG_USAGE_STREAM,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });
    return (void*)(uintptr_t)img.id;
}

void gfx_update_texture(void* h, void* data, int data_byte_size) {
    sg_image img = { .id=(uint32_t)(uintptr_t)h };
    sg_update_image(img, &(sg_image_data){.subimage[0][0] = { .ptr = data, .size=data_byte_size } });
}

void gfx_destroy_texture(void* h) {
    sg_image img = { .id=(uint32_t)(uintptr_t)h };
    sg_destroy_image(img);
}
#endif /* COMMON_IMPL */
