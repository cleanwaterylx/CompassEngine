#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(set = 0, binding = 0) uniform sampler2D in_scene_color;
layout(set = 0, binding = 1) uniform sampler2D in_gbuffer_metallic_roughness;
layout(set = 0, binding = 2) uniform sampler2D in_gbuffer_position;
layout(set = 0, binding = 3) uniform sampler2D in_gbuffer_normal;
layout(set = 0, binding = 4) readonly buffer ssr_frame_buffer
{
    highp mat4 proj_matrix;
    highp mat4 view_matrix;
    highp vec4 _unused_kernel[ssao_kernel_size];
    int        offset_x;
    int        offset_y;
    int        width;
    int        height;
};

layout(location = 0) in highp vec2 in_texcoord;

layout(location = 0) out highp vec4 outFragColor;

highp vec2 ndcxy_to_uv(highp vec2 ndcxy) { return ndcxy * vec2(0.5, 0.5) + vec2(0.5, 0.5); }

highp float Pow5(highp float x)
{
    highp float x2 = x * x;
    return x2 * x2 * x;
}

bool project_to_texcoord(highp vec3 view_position,
                         highp vec2 scale,
                         highp vec2 offset,
                         out highp vec2 projected_uv,
                         out highp vec2 projected_texcoord)
{
    highp vec4 clip_position = proj_matrix * vec4(view_position, 1.0);
    if (clip_position.w <= 0.0)
    {
        return false;
    }

    highp vec3 ndc_position = clip_position.xyz / clip_position.w;
    if (ndc_position.z < 0.0 || ndc_position.z > 1.0)
    {
        return false;
    }

    projected_uv = ndcxy_to_uv(ndc_position.xy);
    if (projected_uv.x < 0.0 || projected_uv.x > 1.0 || projected_uv.y < 0.0 || projected_uv.y > 1.0)
    {
        return false;
    }

    projected_texcoord = projected_uv * scale + offset;
    return true;
}

void main()
{
    highp ivec2 tex_dim = textureSize(in_scene_color, 0);
    highp vec2  scale   = vec2(float(width) / float(tex_dim.x), float(height) / float(tex_dim.y));
    highp vec2  offset  = vec2(float(offset_x) / float(tex_dim.x), float(offset_y) / float(tex_dim.y));
    highp vec2  texcoord = in_texcoord * scale + offset;

    highp vec3 scene_color = texture(in_scene_color, texcoord).rgb;

    highp vec4 gbuffer_metallic_roughness = texture(in_gbuffer_metallic_roughness, texcoord);
    highp float metallic  = gbuffer_metallic_roughness.r;
    highp float roughness = gbuffer_metallic_roughness.b;
    if (roughness >= ssr_roughness_threshold)
    {
        outFragColor = vec4(scene_color, 1.0);
        return;
    }

    highp vec3 view_position = texture(in_gbuffer_position, texcoord).xyz;
    highp vec3 view_normal   = normalize(texture(in_gbuffer_normal, texcoord).xyz);

    if (abs(view_position.z) < 0.0001 || dot(view_normal, view_normal) < 0.5)
    {
        outFragColor = vec4(scene_color, 1.0);
        return;
    }

    highp vec3 V = normalize(-view_position);
    highp vec3 R = normalize(reflect(-V, view_normal));

    highp float march_t             = ssr_step_stride;
    highp float previous_march_t    = 0.0;
    highp float previous_depth_delta = -1.0;
    bool        has_previous_sample = false;
    bool        has_hit             = false;
    highp vec2  hit_texcoord        = texcoord;
    highp vec2  hit_uv              = vec2(0.5, 0.5);
    highp float hit_distance        = 0.0;

    highp vec3 origin = view_position + view_normal * ssr_ray_bias;

    for (int step_index = 0; step_index < ssr_max_steps; ++step_index)
    {
        highp vec3 sample_position = origin + R * march_t;
        if (length(sample_position - view_position) > ssr_max_distance)
        {
            break;
        }

        highp vec2 projected_uv;
        highp vec2 projected_texcoord;
        if (!project_to_texcoord(sample_position, scale, offset, projected_uv, projected_texcoord))
        {
            break;
        }

        highp vec3 scene_position = texture(in_gbuffer_position, projected_texcoord).xyz;
        if (abs(scene_position.z) < 0.0001)
        {
            march_t += ssr_step_stride;
            continue;
        }

        highp float depth_delta = scene_position.z - sample_position.z;
        if (depth_delta > 0.0 && depth_delta < ssr_thickness)
        {
            has_hit      = true;
            hit_texcoord = projected_texcoord;
            hit_uv       = projected_uv;
            hit_distance = march_t;
            break;
        }

        if (has_previous_sample && previous_depth_delta < 0.0 && depth_delta > 0.0)
        {
            highp float binary_min_t = previous_march_t;
            highp float binary_max_t = march_t;
            highp vec2  binary_hit_texcoord = projected_texcoord;
            highp vec2  binary_hit_uv       = projected_uv;

            for (int binary_step = 0; binary_step < ssr_binary_search_steps; ++binary_step)
            {
                highp float binary_mid_t = 0.5 * (binary_min_t + binary_max_t);
                highp vec3  binary_sample_position = origin + R * binary_mid_t;
                highp vec2  binary_projected_uv;
                highp vec2  binary_projected_texcoord;
                if (!project_to_texcoord(binary_sample_position,
                                         scale,
                                         offset,
                                         binary_projected_uv,
                                         binary_projected_texcoord))
                {
                    binary_max_t = binary_mid_t;
                    continue;
                }

                highp vec3 binary_scene_position = texture(in_gbuffer_position, binary_projected_texcoord).xyz;
                if (abs(binary_scene_position.z) < 0.0001)
                {
                    binary_min_t = binary_mid_t;
                    continue;
                }

                highp float binary_depth_delta   = binary_scene_position.z - binary_sample_position.z;
                binary_hit_texcoord              = binary_projected_texcoord;
                binary_hit_uv                    = binary_projected_uv;

                if (binary_depth_delta > 0.0)
                {
                    binary_max_t = binary_mid_t;
                }
                else
                {
                    binary_min_t = binary_mid_t;
                }
            }

            has_hit      = true;
            hit_texcoord = binary_hit_texcoord;
            hit_uv       = binary_hit_uv;
            hit_distance = 0.5 * (binary_min_t + binary_max_t);
            break;
        }

        has_previous_sample  = true;
        previous_march_t     = march_t;
        previous_depth_delta = depth_delta;
        march_t += ssr_step_stride;
    }

    if (!has_hit)
    {
        outFragColor = vec4(scene_color, 1.0);
        return;
    }

    highp vec3 reflection_color = texture(in_scene_color, hit_texcoord).rgb;
    highp float NoV             = clamp(dot(view_normal, V), 0.0, 1.0);
    highp float gloss           = Pow5(clamp(1.0 - roughness, 0.0, 1.0));
    highp float F0              = mix(0.04, 1.0, metallic);
    highp float fresnel         = F0 + (1.0 - F0) * Pow5(1.0 - NoV);
    highp float edge_fade       = clamp(min(hit_uv.x, min(hit_uv.y, min(1.0 - hit_uv.x, 1.0 - hit_uv.y))) * 4.0,
                                  0.0,
                                  1.0);
    highp float distance_fade   = clamp(1.0 - hit_distance / ssr_max_distance, 0.0, 1.0);
    highp float hit_weight      = edge_fade * distance_fade;
    highp float reflection_alpha = clamp(gloss * fresnel * hit_weight * 3.0, 0.0, 1.0);

    outFragColor = vec4(mix(scene_color, reflection_color, clamp(reflection_alpha, 0.0, 1.0)), 1.0);
}
