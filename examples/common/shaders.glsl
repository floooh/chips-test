@vs offscreen_vs
layout(location=0) in vec2 in_pos;
layout(location=1) in vec2 in_uv;

uniform offscreen_vs_params {
    vec2 uv_offset;
    vec2 uv_scale;
};

out vec2 uv;
void main() {
    gl_Position = vec4(in_pos*2.0-1.0, 0.5, 1.0);
    uv = (in_uv * uv_scale) + uv_offset;
}
@end

@fs offscreen_fs
uniform sampler2D fb_tex;
in vec2 uv;
out vec4 frag_color;
void main() {
    frag_color = texture(fb_tex, uv);
}
@end

// offscreen shader with color palette decoding
@fs offscreen_pal_fs
uniform sampler2D fb_tex;
uniform sampler2D pal_tex;
in vec2 uv;
out vec4 frag_color;
void main() {
    float pix = texture(fb_tex, uv).x;
    frag_color = vec4(texture(pal_tex, vec2(pix,0)).xyz, 1.0);
}
@end

@vs display_vs
layout(location=0) in vec2 in_pos;
layout(location=1) in vec2 in_uv;
out vec2 uv;
void main() {
    gl_Position = vec4(in_pos*2.0-1.0, 0.5, 1.0);
    uv = in_uv;
}
@end

@fs display_fs
uniform sampler2D tex;
in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = vec4(texture(tex, uv).xyz, 1.0);
}
@end

@program offscreen offscreen_vs offscreen_fs
@program offscreen_pal offscreen_vs offscreen_pal_fs
@program display display_vs display_fs
