#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_time.h"
#include "gfx.h"

extern const char* vs_src; 
extern const char* fs_src;

static const sg_pass_action pass_action = { .colors[0].action = SG_ACTION_DONTCARE };
static sg_draw_state draw_state;
static int fb_width;
static int fb_height;

uint32_t rgba8_buffer[GFX_MAX_FB_WIDTH * GFX_MAX_FB_HEIGHT];

void gfx_init(int w, int h) {
    fb_width = w;
    fb_height = h;
    sg_setup(&(sg_desc){
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view
    });
    stm_setup();

    float quad_vertices[] = { 0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f };
    draw_state.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(quad_vertices),
        .content = quad_vertices,
    });
    sg_shader fsq_shd = sg_make_shader(&(sg_shader_desc){
        .fs.images = {
            [0] = { .name="tex", .type=SG_IMAGETYPE_2D },
        },
        .vs.source = vs_src,
        .fs.source = fs_src,
    });
    draw_state.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            .attrs[0] = { .name="pos", .format=SG_VERTEXFORMAT_FLOAT2 }
        },
        .shader = fsq_shd,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP
    });
    draw_state.fs_images[0] = sg_make_image(&(sg_image_desc){
        .width = fb_width,
        .height = fb_height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage = SG_USAGE_STREAM,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });
}

void gfx_draw() {
    sg_update_image(draw_state.fs_images[0], &(sg_image_content){
        .subimage[0][0] = { 
            .ptr = rgba8_buffer,
            .size = fb_width*fb_height*sizeof(uint32_t)
        }
    });
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    sg_apply_draw_state(&draw_state);
    sg_draw(0, 4, 1);
    sg_end_pass();
    sg_commit();
}

void gfx_shutdown() {
    sg_shutdown();
}

/* shader source code for GL, GLES2, Metal and D3D11 */
#if defined(SOKOL_GLCORE33)
#error "FIXME: SOKOL_GLCORE33"
/*
const char* vs_src =
    "#version 330\n"
    "in vec2 pos;\n"
    "out vec2 uv;\n"
    "void main() {\n"
    "  gl_Position = vec4(pos*2.0-1.0, 0.5, 1.0);\n"
    "  uv0 = vec2(pos.x, 1.0-pos.y);\n"
    "}\n",
const char* fs_src =
    "#version 330\n"
    "uniform sampler2D tex;\n"
    "in vec2 uv;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "  frag_color = texture(tex, uv);\n"
    "}\n"
*/
#elif defined(SOKOL_GLES2)
const char* vs_src =
    "attribute vec2 pos;\n"
    "varying vec2 uv;\n"
    "void main() {\n"
    "  gl_Position = vec4(pos*2.0-1.0, 0.5, 1.0);\n"
    "  uv = vec2(pos.x, 1.0-pos.y);\n"
    "}\n";
const char* fs_src =
    "precision mediump float;\n"
    "uniform sampler2D tex;"
    "varying vec2 uv;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(tex, uv);\n"
    "}\n";
#elif defined(SOKOL_METAL)
const char* vs_src =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "  float2 pos [[attribute(0)]];\n"
    "};\n"
    "struct vs_out {\n"
    "  float4 pos [[position]];\n"
    "  float2 uv;\n"
    "};\n"
    "vertex vs_out _main(vs_in in [[stage_in]]) {\n"
    "  vs_out out;\n"
    "  out.pos = float4(in.pos*2.0-1.0, 0.5, 1.0);\n"
    "  out.uv = float2(in.pos.x, 1.0-in.pos.y);\n"
    "  return out;\n"
    "}\n";
const char* fs_src =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "  float2 uv;\n"
    "};\n"
    "fragment float4 _main(fs_in in [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) {\n"
    "  return tex.sample(smp, in.uv);\n"
    "}\n";
#elif defined(SOKOL_D3D11)
#error "FIXME: SOKOL_D3D11"
const char* vs_src = "";
const char* fs_src = "";
#endif
