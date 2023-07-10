#version 460

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_texcoord0;
layout(location = 2) in vec2 in_texcoord1;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D base_color_tex;
layout(set = 0, binding = 1) uniform sampler2D metallic_roughness_tex;
layout(set = 0, binding = 2) uniform sampler2D normal_tex;
layout(set = 0, binding = 3) uniform sampler2D occlusion_tex;
layout(set = 0, binding = 4) uniform sampler2D emissive_tex;

layout(set = 0, binding = 5) uniform _unused_name_per_material {
    uint  alpha_mode;
    float alpha_cutoff;
    float metallic_factor;
    float roughness_factor;
    vec4  base_color_factor;
    vec4  emissive_factor;
    uint  texcoord_sets[5];
};

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
    vec3 color = texture(occlusion_tex, in_texcoord0).xyz;
    out_color  = vec4(color * ambient_color.xyz, 1.0);
}