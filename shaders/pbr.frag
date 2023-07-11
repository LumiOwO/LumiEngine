#version 460

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec2 in_texcoord0;
layout(location = 4) in vec2 in_texcoord1;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D base_color_tex;
layout(set = 0, binding = 1) uniform sampler2D metallic_roughness_tex;
layout(set = 0, binding = 2) uniform sampler2D normal_tex;
layout(set = 0, binding = 3) uniform sampler2D occlusion_tex;
layout(set = 0, binding = 4) uniform sampler2D emissive_tex;

layout(set = 0, binding = 5) uniform _unused_name_material {
    int texcoord_set_base_color;
    int texcoord_set_metallic_roughness;
    int texcoord_set_normal;
    int texcoord_set_occlusion;
    int texcoord_set_emissive;
    int _padding_texcoord_set_0;
    int _padding_texcoord_set_1;
    int _padding_texcoord_set_2;

    int   alpha_mode;
    float alpha_cutoff;
    float metallic_factor;
    float roughness_factor;
    vec4  base_color_factor;
    vec4  emissive_factor;
};

layout(set = 1, binding = 0) readonly buffer _unused_name_camera {
    mat4 view;
    mat4 proj;
    mat4 proj_view;
    vec3 cam_pos;
};

layout(set = 1, binding = 1) readonly buffer _unused_name_environment {
    vec4  sunlight_color;
    vec3  sunlight_dir;
    float _padding_sunlight_dir;

    int debug_idx;
};

const float kPi           = 3.141592653589793;
const float kMinRoughness = 0.04;

vec3 Uncharted2Tonemap(vec3 color) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((color * (A * color + C * B) + D * E) /
            (color * (A * color + B) + D * F)) -
           E / F;
}

void main() {
    vec4 base_color_tex_value =
        texture(base_color_tex,
                texcoord_set_base_color <= 0 ? in_texcoord0 : in_texcoord1)
            .rgba;
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
    float metallic_tex_value =
        texture(metallic_roughness_tex, texcoord_set_metallic_roughness <= 0
                                            ? in_texcoord0
                                            : in_texcoord1)
            .b;
    float roughness_tex_value =
        texture(metallic_roughness_tex, texcoord_set_metallic_roughness <= 0
                                            ? in_texcoord0
                                            : in_texcoord1)
            .g;
    vec3 normal_tex_value =
        texture(normal_tex,
                texcoord_set_normal <= 0 ? in_texcoord0 : in_texcoord1)
            .rgb;
    float occlusion_tex_value =
        texture(occlusion_tex,
                texcoord_set_occlusion <= 0 ? in_texcoord0 : in_texcoord1)
            .r;
    vec3 emissive_tex_value =
        texture(emissive_tex,
                texcoord_set_emissive <= 0 ? in_texcoord0 : in_texcoord1)
            .rgb;

    vec3 color = vec3(occlusion_tex_value);
    out_color  = vec4(color * sunlight_color.xyz, 1.0);

    switch (debug_idx) {
        case 0:
            break;
        case 1:
            out_color = base_color_tex_value;
            break;
        case 2:
            out_color = vec4(metallic_tex_value);
            break;
        case 3:
            out_color = vec4(roughness_tex_value);
            break;
        case 4:
            out_color = vec4(normal_tex_value, 1.0);
            break;
        case 5:
            out_color = vec4(occlusion_tex_value);
            break;
        case 6:
            out_color = vec4(emissive_tex_value, 1.0);
            break;
    }
}