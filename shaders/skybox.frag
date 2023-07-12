#version 460

layout(set = 0, binding = 0) uniform samplerCube skybox_cubemap;

layout(location = 0) in vec3 sample_position;

layout(location = 0) out vec4 out_color;

// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Uncharted2Tonemap(vec3 color) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color * (A * color + C * B) + D * E) /
            (color * (A * color + B) + D * F)) -
           E / F;
}

vec3 Tonemap(vec3 color) {
    const float W = 11.2;

    float exposure = 4.5;
    vec3  outcol   = Uncharted2Tonemap(color.rgb * exposure);
    outcol         = outcol * (1.0 / Uncharted2Tonemap(vec3(W)));
    return outcol;
}

void main() {
    vec3 color = textureLod(skybox_cubemap, sample_position, 0.0).rgb;
    color = Tonemap(color);
    out_color  = vec4(color, 1.0);
}