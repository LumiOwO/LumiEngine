#version 460

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_texcoord;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D diffuse_tex;

layout(set = 1, binding = 0) readonly buffer _unused_name_per_frame {
    mat4 view;
    mat4 proj;
    mat4 proj_view;
    vec4 fog_color;      // w is for exponent
    vec4 fog_distances;  // x for min, y for max, zw unused.
    vec4 ambient_color;
    vec4 sunlight_direction;  // w for sun power
    vec4 sunlight_color;
};

void main() {
    vec3 color = texture(diffuse_tex, in_texcoord).xyz;
    out_color  = vec4(color * ambient_color.xyz, 1.0);
}