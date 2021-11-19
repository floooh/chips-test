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
    int border_top;
    int border_bottom;
    int border_left;
    int border_right;
    int emu_aspect_x;
    int emu_aspect_y;
    bool rot90;
    void (*draw_extra_cb)(void);
} gfx_desc_t;

void gfx_init(const gfx_desc_t* desc);
uint32_t* gfx_framebuffer(void);
size_t gfx_framebuffer_size(void);
void gfx_draw(int emu_width, int emu_height);
void gfx_shutdown(void);
void* gfx_create_texture(int w, int h);
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
#include "sokol_glue.h"
#include "shaders.glsl.h"
#include <assert.h>

#define _GFX_DEF(v,def) (v?v:def)

typedef struct {
    bool valid;
    sg_pipeline upscale_pip;
    sg_bindings upscale_bind;
    sg_pass upscale_pass;
    sg_pipeline display_pip;
    sg_bindings display_bind;
    sg_pass_action upscale_pass_action;
    sg_pass_action draw_pass_action;
    int flash_success_count;
    int flash_error_count;
    int border_top;
    int border_bottom;
    int border_left;
    int border_right;
    int emu_aspect_x;
    int emu_aspect_y;
    int emu_width;
    int emu_height;
    bool rot90;
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

void gfx_flash_success(void) {
    assert(gfx.valid);
    gfx.flash_success_count = 20;
}

void gfx_flash_error(void) {
    assert(gfx.valid);
    gfx.flash_error_count = 20;
}

uint32_t* gfx_framebuffer(void) {
    assert(gfx.valid);
    return gfx.rgba8_buffer;
}

size_t gfx_framebuffer_size(void) {
    assert(gfx.valid);
    return sizeof(gfx.rgba8_buffer);
}

static void gfx_init_images_and_pass(void) {

    /* destroy previous resources (if exist) */
    sg_destroy_image(gfx.upscale_bind.fs_images[0]);
    sg_destroy_image(gfx.display_bind.fs_images[0]);
    sg_destroy_pass(gfx.upscale_pass);

    /* a texture with the emulator's raw pixel data */
    gfx.upscale_bind.fs_images[0] = sg_make_image(&(sg_image_desc){
        .width = gfx.emu_width,
        .height = gfx.emu_height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = SG_USAGE_STREAM,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });
    /* a 2x upscaled render-target-texture */
    gfx.display_bind.fs_images[0] = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 2 * gfx.emu_width,
        .height = 2 * gfx.emu_height,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });

    /* a render pass for the 2x upscaling */
    gfx.upscale_pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = gfx.display_bind.fs_images[0]
    });
}

void gfx_init(const gfx_desc_t* desc) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 8,
        .image_pool_size = 128,
        .shader_pool_size = 4,
        .pipeline_pool_size = 4,
        .context_pool_size = 2,
        .context = sapp_sgcontext()
    });
    sdtx_setup(&(sdtx_desc_t){
        .context_pool_size = 1,
        .fonts[0] = sdtx_font_oric()
    });

    gfx = (gfx_state_t) {
        .valid = true,
        .upscale_pass_action = {
            .colors[0] = { .action = SG_ACTION_DONTCARE }
        },
        .draw_pass_action = {
            .colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.05f, 0.05f, 0.05f, 1.0f } }
        },
        .border_top = desc->border_top,
        .border_bottom = desc->border_bottom,
        .border_left = desc->border_left,
        .border_right = desc->border_right,
        .emu_width = 0,
        .emu_height = 0,
        .emu_aspect_x = _GFX_DEF(desc->emu_aspect_x, 1),
        .emu_aspect_y = _GFX_DEF(desc->emu_aspect_y, 1),
        .rot90 = desc->rot90,
        .draw_extra_cb = desc->draw_extra_cb,

        .upscale_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
            .data = SG_RANGE(gfx_verts)
        }),

        .display_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
            .data = {
                .ptr = sg_query_features().origin_top_left ?
                        (desc->rot90 ? gfx_verts_rot : gfx_verts) :
                        (desc->rot90 ? gfx_verts_flipped_rot : gfx_verts_flipped),
                .size = sizeof(gfx_verts)
            }
        }),
    
        .display_pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = sg_make_shader(display_shader_desc(sg_query_backend())),
            .layout = {
                .attrs = {
                    [0].format = SG_VERTEXFORMAT_FLOAT2,
                    [1].format = SG_VERTEXFORMAT_FLOAT2
                }
            },
            .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP
        }),

        .upscale_pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader = sg_make_shader(upscale_shader_desc(sg_query_backend())),
            .layout = {
                .attrs = {
                    [0].format = SG_VERTEXFORMAT_FLOAT2,
                    [1].format = SG_VERTEXFORMAT_FLOAT2
                }
            },
            .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
            .depth.pixel_format = SG_PIXELFORMAT_NONE
        }),
    };
}

/* apply a viewport rectangle to preserve the emulator's aspect ratio,
   and for 'portrait' orientations, keep the emulator display at the
   top, to make room at the bottom for mobile virtual keyboard
*/
static void apply_viewport(int canvas_width, int canvas_height) {
    float cw = canvas_width - gfx.border_left - gfx.border_right;
    if (cw < 1.0f) {
        cw = 1.0f;
    }
    float ch = canvas_height - gfx.border_top - gfx.border_bottom;
    if (ch < 1.0f) {
        ch = 1.0f;
    }
    const float canvas_aspect = (float)cw / (float)ch;
    const float emu_aspect = (float)(gfx.emu_width*gfx.emu_aspect_x) / (float)(gfx.emu_height*gfx.emu_aspect_y);
    float vp_x, vp_y, vp_w, vp_h;
    if (emu_aspect < canvas_aspect) {
        vp_y = gfx.border_top;
        vp_h = ch;
        vp_w = (ch * emu_aspect);
        vp_x = gfx.border_left + (cw - vp_w) / 2;
    }
    else {
        vp_x = gfx.border_left;
        vp_w = cw;
        vp_h = (cw / emu_aspect);
        vp_y = gfx.border_top;
    }
    sg_apply_viewport(vp_x, vp_y, vp_w, vp_h, true);
}

void gfx_draw(int emu_width, int emu_height) {
    assert(gfx.valid);
    /* check if framebuffer size has changed, need to create new backing texture */
    if ((emu_width != gfx.emu_width) || (emu_height != gfx.emu_height)) {
        gfx.emu_width = emu_width;
        gfx.emu_height = emu_height;
        gfx_init_images_and_pass();
    }

    /* copy emulator pixel data into upscaling source texture */
    sg_update_image(gfx.upscale_bind.fs_images[0], &(sg_image_data){
        .subimage[0][0] = {
            .ptr = gfx.rgba8_buffer,
            .size = gfx.emu_width*gfx.emu_height*sizeof(uint32_t)
        }
    });

    /* upscale the original framebuffer 2x with nearest filtering */
    sg_begin_pass(gfx.upscale_pass, &gfx.upscale_pass_action);
    sg_apply_pipeline(gfx.upscale_pip);
    sg_apply_bindings(&gfx.upscale_bind);
    sg_draw(0, 4, 1);
    sg_end_pass();

    /* tint the clear color red or green if flash feedback is requested */
    if (gfx.flash_error_count > 0) {
        gfx.flash_error_count--;
        gfx.draw_pass_action.colors[0].value.r = 0.7f;
    }
    else if (gfx.flash_success_count > 0) {
        gfx.flash_success_count--;
        gfx.draw_pass_action.colors[0].value.g = 0.7f;
    }
    else {
        gfx.draw_pass_action.colors[0].value.r = 0.05f;
        gfx.draw_pass_action.colors[0].value.g = 0.05f;
    }

    /* draw the final pass with linear filtering */
    int w = (int) sapp_width();
    int h = (int) sapp_height();
    sg_begin_default_pass(&gfx.draw_pass_action, w, h);
    apply_viewport(w, h);
    sg_apply_pipeline(gfx.display_pip);
    sg_apply_bindings(&gfx.display_bind);
    sg_draw(0, 4, 1);
    sg_apply_viewport(0, 0, w, h, true);
    sdtx_draw();
    if (gfx.draw_extra_cb) {
        gfx.draw_extra_cb();
    }
    sg_end_pass();
    sg_commit();
}

void gfx_shutdown() {
    assert(gfx.valid);
    sdtx_shutdown();
    sg_shutdown();
}

void* gfx_create_texture(int w, int h) {
    sg_image img = sg_make_image(&(sg_image_desc){
        .width = w,
        .height = h,
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
