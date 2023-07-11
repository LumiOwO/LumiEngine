#version 460

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec2 in_texcoord0;
layout(location = 4) in vec2 in_texcoord1;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_color;
layout(location = 3) out vec2 out_texcoord0;
layout(location = 4) out vec2 out_texcoord1;

layout(set = 1, binding = 0) readonly buffer _unused_name_camera {
    mat4 view;
    mat4 proj;
    mat4 proj_view;
    vec3 cam_pos;
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
    mat4 world_to_object = visible_objects[gl_InstanceIndex].world_to_object;

    out_position = (object_to_world * vec4(in_position, 1.0)).xyz;
    gl_Position  = proj_view * vec4(out_position, 1.0);

    out_normal = transpose(mat3(world_to_object)) * in_normal;

    out_color     = in_color;
    out_texcoord0 = in_texcoord0;
    out_texcoord1 = in_texcoord1;
}