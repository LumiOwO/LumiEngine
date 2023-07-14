#version 460

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec2 in_texcoord0;
layout(location = 4) in vec2 in_texcoord1;

layout(set = 1, binding = 1) readonly buffer _unused_name_environment {
    vec3  sunlight_color;
    float sunlight_intensity;
    vec3  sunlight_dir;
    float ibl_intensity;
    float mip_levels;
    int   debug_idx;
    float _padding_0;
    float _padding_1;

    mat4 sunlight_world_to_clip;
};

struct MeshInstanceData {
    mat4 object_to_world;
    mat4 world_to_object;
};

layout(set = 2, binding = 0) readonly buffer _unused_name_mesh_instance {
    MeshInstanceData visible_objects[];
};

void main() {
    mat4 object_to_world = visible_objects[gl_InstanceIndex].object_to_world;

    vec3 world_position = (object_to_world * vec4(in_position, 1.0)).xyz;
    gl_Position         = sunlight_world_to_clip * vec4(world_position, 1.0);
}