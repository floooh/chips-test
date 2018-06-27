#pragma once
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
