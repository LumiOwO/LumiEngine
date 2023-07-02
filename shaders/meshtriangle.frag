#version 460

layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 in_texcoord;

layout (location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform _unused_name_env_lighting_buffer
{
    vec4 fog_color;      // w is for exponent
    vec4 fog_distances;  // x for min, y for max, zw unused.
    vec4 ambient_color;
    vec4 sunlight_direction;  // w for sun power
    vec4 sunlight_color;
};

layout(set = 2, binding = 0) uniform sampler2D tex1;

void main()
{
    vec3 color = texture(tex1, in_texcoord).xyz;
    out_color  = vec4(color * ambient_color.xyz, 1.0);
}