@vs upscale_vs
layout(location=0) in vec2 in_pos;
layout(location=1) in vec2 in_uv;
out vec2 uv;
void main() {
    gl_Position = vec4(in_pos*2.0-1.0, 0.5, 1.0);
    uv = in_uv;
}
@end

@fs upscale_fs
layout(binding=0) uniform sampler2D tex;
in vec2 uv;
out vec4 frag_color;
void main() {
    frag_color = texture(tex, uv);
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
layout(binding=0) uniform sampler2D tex;
layout(binding=0) uniform display_params {
    vec2 dim;
};

in vec2 uv;
out vec4 frag_color;

vec3 calc_mask(vec2 uv) {
    float iy = mod(gl_FragCoord.y * 0.5, 1.0);
    return vec3(iy * 1.25);
}

void main() {
    vec3 mask = calc_mask(uv);
    vec3 c = texture(tex, uv).xyz * mask;
    frag_color = vec4(c, 1.0);
}
@end

@program upscale upscale_vs upscale_fs
@program display display_vs display_fs


