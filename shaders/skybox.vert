#version 460

layout(set = 1, binding = 0) readonly buffer _unused_name_camera {
    mat4 view;
    mat4 proj;
    mat4 proj_view;
    vec3 cam_pos;
};

layout(location = 0) out vec3 out_uvw;

void main() {
    const vec3 vertex_offsets[8] = vec3[8](  //
        vec3(1.0, 1.0, 1.0),                 //
        vec3(1.0, 1.0, -1.0),                //
        vec3(1.0, -1.0, -1.0),               //
        vec3(1.0, -1.0, 1.0),                //
        vec3(-1.0, 1.0, 1.0),                //
        vec3(-1.0, 1.0, -1.0),               //
        vec3(-1.0, -1.0, -1.0),              //
        vec3(-1.0, -1.0, 1.0)                //
    );

    // x+, y+, x-, y-, z+, z-
    const int indices[36] = int[36](  //
        0, 1, 2,                      //
        2, 3, 0,                      //
        4, 5, 1,                      //
        1, 0, 4,                      //
        7, 6, 5,                      //
        5, 4, 7,                      //
        3, 2, 6,                      //
        6, 7, 3,                      //
        4, 0, 3,                      //
        3, 7, 4,                      //
        1, 5, 6,                      //
        6, 2, 1                       //
    );

    // vec3 world_position = camera_position + (camera_z_far_plane / 1.733) *
    // cube_corner_vertex_offsets[cube_triangle_index[gl_VertexIndex]];
    vec3 world_position = cam_pos + vertex_offsets[indices[gl_VertexIndex]];

    // world to NDC
    vec4 clip_position = proj_view * vec4(world_position, 1.0);
    // depth set to 0.99999?
    // clip_position.z = clip_position.w * 0.99999;
    gl_Position = clip_position;

    out_uvw = normalize(world_position - cam_pos);
}