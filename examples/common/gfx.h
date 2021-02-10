#pragma once
/* 
    Common graphics functions for the chips-test example emulators.

    REMINDER: consider using this CRT shader?

    https://github.com/mattiasgustavsson/rebasic/blob/master/source/libs/crtemu.h
*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GFX_MAX_FB_WIDTH (1024)
#define GFX_MAX_FB_HEIGHT (1024)

typedef struct {
    int top_offset;
    int aspect_x;
    int aspect_y;
    bool rot90;
    void (*draw_extra_cb)(void);
} gfx_desc_t;

void gfx_init(const gfx_desc_t* desc);
uint32_t* gfx_framebuffer(void);
int gfx_framebuffer_size(void);
void gfx_draw(int width, int height);
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
#include "sokol_time.h"
#include "sokol_glue.h"
#include "shaders.glsl.h"

#define _GFX_DEF(v,def) (v?v:def)

static struct {
    sg_pipeline upscale_pip;
    sg_bindings upscale_bind;
    sg_pass upscale_pass;
    sg_pipeline display_pip;
    sg_bindings display_bind;
    sg_pass_action upscale_pass_action;
    sg_pass_action draw_pass_action;
    int flash_success_count;
    int flash_error_count;
    int top_offset;
    int fb_width;
    int fb_height;
    int fb_aspect_x;
    int fb_aspect_y;
    bool rot90;
    uint32_t rgba8_buffer[GFX_MAX_FB_WIDTH * GFX_MAX_FB_HEIGHT];
    void (*draw_extra_cb)(void);
} gfx;

void gfx_flash_success(void) {
    gfx.flash_success_count = 20;
}

void gfx_flash_error(void) {
    gfx.flash_error_count = 20;
}

uint32_t* gfx_framebuffer(void) {
    return gfx.rgba8_buffer;
}

int gfx_framebuffer_size(void) {
    return sizeof(gfx.rgba8_buffer);
}

void gfx_init_images_and_pass(void) {

    /* destroy previous resources (if exist) */
    sg_destroy_image(gfx.upscale_bind.fs_images[0]);
    sg_destroy_image(gfx.display_bind.fs_images[0]);
    sg_destroy_pass(gfx.upscale_pass);

    /* a texture with the emulator's raw pixel data */
    gfx.upscale_bind.fs_images[0] = sg_make_image(&(sg_image_desc){
        .width = gfx.fb_width,
        .height = gfx.fb_height,
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
        .width = 2 * gfx.fb_width,
        .height = 2 * gfx.fb_height,
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

    gfx.upscale_pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_DONTCARE }
    };
    gfx.draw_pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.05f, 0.05f, 0.05f, 1.0f } }
    };

    gfx.top_offset = desc->top_offset;
    gfx.fb_width = 0;
    gfx.fb_height = 0;
    gfx.fb_aspect_x = _GFX_DEF(desc->aspect_x, 1);
    gfx.fb_aspect_y = _GFX_DEF(desc->aspect_y, 1);
    gfx.rot90 = desc->rot90;
    gfx.draw_extra_cb = desc->draw_extra_cb;
    sg_setup(&(sg_desc){
        .buffer_pool_size = 8,
        .image_pool_size = 128,
        .shader_pool_size = 4,
        .pipeline_pool_size = 4,
        .context_pool_size = 2,
        .context = sapp_sgcontext()
    });

    /* quad vertex buffers with and without flipped UVs */
    static float verts[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f
    };
    static float verts_rot[] = {
        0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f
    };
    static float verts_flipped[] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 0.0f
    };
    static float verts_flipped_rot[] = {
        0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 0.0f
    };
    gfx.upscale_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(verts)
    });
    gfx.display_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = {
            .ptr = sg_query_features().origin_top_left ?
                    (gfx.rot90 ? verts_rot : verts) :
                    (gfx.rot90 ? verts_flipped_rot : verts_flipped),
            .size = sizeof(verts)
        }
    });

    /* 2 pipeline-state-objects, one for upscaling, one for rendering */
    gfx.display_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(display_shader_desc(sg_query_backend())),
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP
    });
    gfx.upscale_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(upscale_shader_desc(sg_query_backend())),
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .depth.pixel_format = SG_PIXELFORMAT_NONE
    });
}

/* apply a viewport rectangle to preserve the emulator's aspect ratio,
   and for 'portrait' orientations, keep the emulator display at the
   top, to make room at the bottom for mobile virtual keyboard
*/
static void apply_viewport(int canvas_width, int canvas_height) {
    const float canvas_aspect = (float)canvas_width / (float)canvas_height;
    const float fb_aspect = (float)(gfx.fb_width*gfx.fb_aspect_x) / (float)(gfx.fb_height*gfx.fb_aspect_y);
    const int frame_x = 5;
    const int frame_y = 5;
    int vp_x, vp_y, vp_w, vp_h;
    if (fb_aspect < canvas_aspect) {
        vp_y = frame_y + gfx.top_offset;
        vp_h = canvas_height - (2 * frame_y) - gfx.top_offset;
        vp_w = (int) (canvas_height * fb_aspect) - 2 * frame_x;
        vp_x = (canvas_width - vp_w) / 2;
    }
    else {
        vp_x = frame_x;
        vp_w = canvas_width - 2 * frame_x;
        vp_h = (int) (canvas_width / fb_aspect) - (2 * frame_y) - gfx.top_offset;
        vp_y = frame_y + gfx.top_offset;
    }
    sg_apply_viewport(vp_x, vp_y, vp_w, vp_h, true);
}

void gfx_draw(int width, int height) {
    /* check if framebuffer size has changed, need to create new backing texture */
    if ((width != gfx.fb_width) || (height != gfx.fb_height)) {
        gfx.fb_width = width;
        gfx.fb_height = height;
        gfx_init_images_and_pass();
    }

    /* copy emulator pixel data into upscaling source texture */
    sg_update_image(gfx.upscale_bind.fs_images[0], &(sg_image_data){
        .subimage[0][0] = {
            .ptr = gfx.rgba8_buffer,
            .size = gfx.fb_width*gfx.fb_height*sizeof(uint32_t)
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
    if (gfx.draw_extra_cb) {
        gfx.draw_extra_cb();
    }
    sg_end_pass();
    sg_commit();
}

void gfx_shutdown() {
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
