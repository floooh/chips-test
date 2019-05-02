#pragma once
/* 
    Common graphics functions for the chips-test example emulators.
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
#include "shaders.glsl.h"

#define _GFX_DEF(v,def) (v?v:def)

static const sg_pass_action gfx_upscale_pass_action = {
    .colors[0] = { .action = SG_ACTION_DONTCARE }
};
static sg_pass_action gfx_draw_pass_action = {
    .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 0.05f, 0.05f, 0.05f, 1.0f } }
};
typedef struct {
    sg_pipeline upscale_pip;
    sg_bindings upscale_bind;
    sg_pass upscale_pass;
    sg_pipeline draw_pip;
    sg_bindings draw_bind;
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
} gfx_state;
static gfx_state gfx;

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
    sg_destroy_image(gfx.draw_bind.fs_images[0]);
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
    gfx.draw_bind.fs_images[0] = sg_make_image(&(sg_image_desc){
        .render_target = true,
        .width = 2 * gfx.fb_width,
        .height = 2 * gfx.fb_height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });

    /* a render pass for the 2x upscaling */
    gfx.upscale_pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = gfx.draw_bind.fs_images[0]
    });
}

void gfx_init(const gfx_desc_t* desc) {
    gfx.top_offset = desc->top_offset;
    gfx.fb_width = 0;
    gfx.fb_height = 0;
    gfx.fb_aspect_x = _GFX_DEF(desc->aspect_x, 1);
    gfx.fb_aspect_y = _GFX_DEF(desc->aspect_y, 1);
    gfx.rot90 = desc->rot90;
    gfx.draw_extra_cb = desc->draw_extra_cb;
    sg_setup(&(sg_desc){
        .buffer_pool_size = 8,
        .image_pool_size = 8,
        .shader_pool_size = 4,
        .pipeline_pool_size = 4,
        .context_pool_size = 2,
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view
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
        .size = sizeof(verts),
        .content = verts,
    });
    gfx.draw_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(verts),
        .content = sg_query_feature(SG_FEATURE_ORIGIN_TOP_LEFT) ? 
                        (gfx.rot90 ? verts_rot : verts) :
                        (gfx.rot90 ? verts_flipped_rot : verts_flipped)
    });

    /* 2 pipeline-state-objects, one for upscaling, one for rendering */
    sg_pipeline_desc pip_desc = {
        .shader = sg_make_shader(fsq_shader_desc()),
        .layout = {
            .attrs = {
                [vs_in_pos].format = SG_VERTEXFORMAT_FLOAT2,
                [vs_in_uv].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP
    };
    gfx.draw_pip = sg_make_pipeline(&pip_desc);
    pip_desc.blend.depth_format = SG_PIXELFORMAT_NONE;
    gfx.upscale_pip = sg_make_pipeline(&pip_desc);
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
    sg_update_image(gfx.upscale_bind.fs_images[0], &(sg_image_content){
        .subimage[0][0] = { 
            .ptr = gfx.rgba8_buffer,
            .size = gfx.fb_width*gfx.fb_height*sizeof(uint32_t)
        }
    });

    /* upscale the original framebuffer 2x with nearest filtering */
    sg_begin_pass(gfx.upscale_pass, &gfx_upscale_pass_action);
    sg_apply_pipeline(gfx.upscale_pip);
    sg_apply_bindings(&gfx.upscale_bind);
    sg_draw(0, 4, 1);
    sg_end_pass();

    /* tint the clear color red or green if flash feedback is requested */
    if (gfx.flash_error_count > 0) {
        gfx.flash_error_count--;
        gfx_draw_pass_action.colors[0].val[0] = 0.7f;
    }
    else if (gfx.flash_success_count > 0) {
        gfx.flash_success_count--;
        gfx_draw_pass_action.colors[0].val[1] = 0.7f;
    }
    else {
        gfx_draw_pass_action.colors[0].val[0] = 0.05f;
        gfx_draw_pass_action.colors[0].val[1] = 0.05f;
    }

    /* draw the final pass with linear filtering */
    const int canvas_width = sapp_width();
    const int canvas_height = sapp_height();
    sg_begin_default_pass(&gfx_draw_pass_action, canvas_width, canvas_height);
    apply_viewport(canvas_width, canvas_height);
    sg_apply_pipeline(gfx.draw_pip);
    sg_apply_bindings(&gfx.draw_bind);
    sg_draw(0, 4, 1);
    sg_apply_viewport(0, 0, canvas_width, canvas_height, true);
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
    sg_update_image(img, &(sg_image_content){.subimage[0][0] = { .ptr = data, .size=data_byte_size } });
}

void gfx_destroy_texture(void* h) {
    sg_image img = { .id=(uint32_t)(uintptr_t)h };
    sg_destroy_image(img);
}
#endif /* COMMON_IMPL */
