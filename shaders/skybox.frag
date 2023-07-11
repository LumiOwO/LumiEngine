#version 460

layout(set = 0, binding = 0) uniform samplerCube specular_sampler;

layout(location = 0) in vec3 in_uvw;

layout(location = 0) out vec4 out_color;

void main() {
    // vec3 origin_sample_uvw = vec3(in_uvw.x, in_uvw.z, in_uvw.y);
    vec3 color = textureLod(specular_sampler, in_uvw, 0.0).rgb;
    out_color  = vec4(color, 1.0);
}