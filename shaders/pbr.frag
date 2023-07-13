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
    vec3  sunlight_color;
    float sunlight_intensity;
    vec3  sunlight_dir;
    float mip_levels;

    int debug_idx;
};

layout(set = 1, binding = 2) uniform samplerCube skybox_irradiance;
layout(set = 1, binding = 3) uniform samplerCube skybox_specular;
layout(set = 1, binding = 4) uniform sampler2D lut_brdf;

const float kPi           = 3.141592653589793;
const float kOneOverPi    = 1.0 / kPi;
const float kMinRoughness = 0.04;

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

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 UnpackNormal(vec3 packed_normal) {
    // Perturb normal, see http://www.thetenthplanet.de/archives/1180
    vec3 tangent_normal;
    tangent_normal.xy = (packed_normal.xy * 2.0 - 1.0);
    tangent_normal.z =
        sqrt(1.0 - clamp(dot(tangent_normal.xy, tangent_normal.xy), 0.0, 1.0));

    vec3 q1  = dFdx(in_position);
    vec3 q2  = dFdy(in_position);
    vec2 st1 = dFdx(in_texcoord0);
    vec2 st2 = dFdy(in_texcoord0);

    vec3 N = normalize(in_normal);
    vec3 T = normalize(q1 * st2.t - q2 * st1.t);
    vec3 B = -normalize(cross(N, T));

    mat3 tangent_to_world = mat3(T, B, N);
    return normalize(tangent_to_world * tangent_normal);
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
vec3 SpecularReflection(float VdotH, vec3 reflectance0, vec3 reflectance90) {
    return reflectance0 + (reflectance90 - reflectance0) *
                              pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
float GeometricOcclusion(float NdotL, float NdotV, float alpha_roughness) {
    float r = alpha_roughness;

    float attenuationL =
        2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV =
        2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals 
// across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
float MicrofacetDistribution(float NdotH, float alpha_roughness) {
    float roughness_square = alpha_roughness * alpha_roughness;

    float f = (NdotH * roughness_square - NdotH) * NdotH + 1.0;
    return roughness_square / (kPi * f * f);
}

void main() {
    // --- Get texture values ---
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

    // --- Prepare PBR infos ---
    vec4 base_color = base_color_factor * base_color_tex_value;
    if (alpha_mode == 1 && base_color.a < alpha_cutoff) {
        discard;
    }
    base_color.rgb *= in_color;

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness.
    float metallic = clamp(metallic_factor * metallic_tex_value, 0.0, 1.0);
    float perceptual_roughness =
        clamp(roughness_factor * roughness_tex_value, kMinRoughness, 1.0);
    float alpha_roughness = perceptual_roughness * perceptual_roughness;

    vec3 f0            = vec3(0.04);
    vec3 diffuse_color = base_color.rgb * (vec3(1.0) - f0);
    diffuse_color *= 1.0 - metallic;
    vec3 specular_color = mix(f0, base_color.rgb, metallic);

    // Compute reflectance
    float reflectance =
        max(max(specular_color.r, specular_color.g), specular_color.b);
    // For typical incident reflectance range (between 4% to 100%),
    // set the grazing reflectance to 100% for typical fresnel effect.
    // For very low reflectance range on highly diffuse objects (below 4%), 
    // incrementally reduce grazing reflecance to 0%.
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    // full reflectance color (normal incidence angle)
    vec3 specular_env_R0 = specular_color.rgb;
    // reflectance color at grazing angle
    vec3 specular_env_R90 = vec3(1.0, 1.0, 1.0) * reflectance90;

    // Directions
    vec3 n = UnpackNormal(normal_tex_value);    // normal
    vec3 v = normalize(cam_pos - in_position);  // view dir from surface
    vec3 l = normalize(-sunlight_dir);          // light dir from surface
    vec3 h = normalize(l + v);                  // Half vector
    vec3 reflection = reflect(-v, n);

    float NdotL = clamp(dot(n, l), 0.001, 1.0);
    float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    // --- Local illumination ---
    vec3 color = vec3(0.0);

    // Calculate the shading terms for the microfacet specular shading model
    vec3  F = SpecularReflection(VdotH, specular_env_R0, specular_env_R90);
    float G = GeometricOcclusion(NdotL, NdotV, alpha_roughness);
    float D = MicrofacetDistribution(NdotH, alpha_roughness);

    // Calculation of analytical lighting contribution
    vec3 diffuse_contrib  = (1.0 - F) * diffuse_color * kOneOverPi;
    vec3 specular_contrib = F * G * D / (4.0 * NdotL * NdotV);
    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
    // vec3  local_color        = NdotL * (diffuse_contrib + specular_contrib);
    // // float sunlight_intensity = sunlight_color.w;
    // local_color *= sunlight_color.rgb * sunlight_intensity;
    vec3 local_coeff = NdotL * sunlight_intensity * sunlight_color.rgb;
    vec3 local_color = local_coeff * (diffuse_contrib + specular_contrib);
    color += local_color;

    // --- Environment illumination (IBL) ---
    // Calculation of the lighting contribution from an optional Image Based Light source.
    float lod = (perceptual_roughness * mip_levels);

    vec3 ibl_diffuse_light = Tonemap(texture(skybox_irradiance, n).rgb);
    vec3 ibl_diffuse       = ibl_diffuse_light * diffuse_color;

    vec2 ibl_brdf =
        (texture(lut_brdf, vec2(NdotV, 1.0 - perceptual_roughness))).rg;
    vec3 ibl_specular_light =
        Tonemap(textureLod(skybox_specular, reflection, lod).rgb);
    vec3 ibl_specular =
        ibl_specular_light * (specular_color * ibl_brdf.x + ibl_brdf.y);

    vec3 ibl_color = ibl_diffuse + ibl_specular;
    color += ibl_color;

    // Apply ambient occlusion
    float ao = occlusion_tex_value;
    color *= ao;

    // Apply emission
    vec3 emissive = emissive_factor.rgb * emissive_tex_value;
    color += emissive;
    
    // Finally output
    out_color = vec4(color, base_color.a);

    // Debug
    switch (debug_idx) {
        case 0:
            break;
        case 1:
            out_color = vec4(local_coeff * diffuse_contrib, 1.0);
            break;
        case 2:
            out_color = vec4(local_coeff * specular_contrib, 1.0);
            break;
        case 3:
            out_color = vec4(ibl_diffuse, 1.0);
            break;
        case 4:
            out_color = vec4(ibl_specular, 1.0);
            break;
        case 5:
            out_color = base_color_tex_value;
            break;
        case 6:
            out_color = vec4(metallic_tex_value);
            break;
        case 7:
            out_color = vec4(roughness_tex_value);
            break;
        case 8:
            out_color = vec4(normal_tex_value, 1.0);
            break;
        case 9:
            out_color = vec4(occlusion_tex_value);
            break;
        case 10:
            out_color = vec4(emissive_tex_value, 1.0);
            break;
        case 11:
            out_color = vec4(F, 1.0);
            break;
        case 12:
            out_color = vec4(G);
            break;
        case 13:
            out_color = vec4(D);
            break;
        default:
            break;
    }
}