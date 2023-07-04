#version 460

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec2 in_texcoord;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec2 out_texcoord;

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

struct PerVisibleData {
    mat4 model;
};

layout(set = 2, binding = 0) readonly buffer _unused_name_per_visible {
    PerVisibleData visible_objects[];
};

void main() {
    mat4 model   = visible_objects[gl_InstanceIndex].model;
    gl_Position  = proj_view * model * vec4(in_position, 1.0);
    out_color    = in_color;
    out_texcoord = in_texcoord;
}