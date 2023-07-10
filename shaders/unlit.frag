#version 460

layout(location = 0) in vec2 in_texcoord0;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D base_color_tex;

void main() {
    vec3 color = texture(base_color_tex, in_texcoord0).xyz;
    out_color  = vec4(color, 1.0);
}