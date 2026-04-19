#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(set = 0, binding = 0) uniform sampler2D in_scene_color;
layout(set = 0, binding = 1) uniform sampler2D in_gbuffer_metallic_roughness;
layout(set = 0, binding = 2) uniform sampler2D in_gbuffer_position;
layout(set = 0, binding = 3) uniform sampler2D in_gbuffer_normal;
layout(set = 0, binding = 5) uniform highp sampler2D in_hiz;
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

highp float linear_view_depth(highp vec3 view_position) { return -view_position.z; }

highp float thickness_at_depth(highp float linear_depth)
{
    return clamp(ssr_thickness + linear_depth * ssr_thickness_scale, ssr_thickness, ssr_max_thickness);
}

highp float sample_scene_linear_depth(highp vec2 texcoord)
{
    return texture(in_gbuffer_position, texcoord).a;
}

bool is_valid_scene_linear_depth(highp float linear_depth)
{
    return linear_depth > NEAR_PLANE && linear_depth < FAR_PLANE;
}

int get_hiz_max_mip()
{
    highp ivec2 mip_size = textureSize(in_hiz, 0);
    int         max_mip  = 0;
    for (int mip = 0; mip < 16; ++mip)
    {
        if (mip_size.x <= 1 && mip_size.y <= 1)
        {
            break;
        }

        mip_size = max(mip_size / 2, ivec2(1));
        max_mip  = mip + 1;
    }

    return max_mip;
}

int get_viewport_max_mip()
{
    highp ivec2 mip_size = max(ivec2(width, height), ivec2(1));
    int         max_mip  = 0;
    for (int mip = 0; mip < 16; ++mip)
    {
        if (mip_size.x <= 1 && mip_size.y <= 1)
        {
            break;
        }

        mip_size = max(mip_size / 2, ivec2(1));
        max_mip  = mip + 1;
    }

    return max_mip;
}

highp float sample_hiz_linear_depth(highp vec2 texcoord, int mip_level)
{
    return textureLod(in_hiz, texcoord, float(mip_level)).r;
}

bool project_to_uv_unclamped(highp vec3 view_position, out highp vec2 projected_uv, out highp vec4 clip_position)
{
    clip_position = proj_matrix * vec4(view_position, 1.0);
    if (clip_position.w <= 0.0)
    {
        return false;
    }

    projected_uv = ndcxy_to_uv(clip_position.xy / clip_position.w);
    return true;
}

bool project_to_texcoord(highp vec3 view_position,
                         highp vec2 scale,
                         highp vec2 offset,
                         out highp vec2 projected_uv,
                         out highp vec2 projected_texcoord)
{
    highp vec4 clip_position;
    if (!project_to_uv_unclamped(view_position, projected_uv, clip_position))
    {
        return false;
    }

    highp vec3 ndc_position = clip_position.xyz / clip_position.w;
    if (ndc_position.z < 0.0 || ndc_position.z > 1.0)
    {
        return false;
    }

    if (projected_uv.x < 0.0 || projected_uv.x > 1.0 || projected_uv.y < 0.0 || projected_uv.y > 1.0)
    {
        return false;
    }

    projected_texcoord = projected_uv * scale + offset;
    return true;
}

highp vec3 sample_segment_view_position(highp vec3 q0,
                                        highp vec3 q1,
                                        highp float k0,
                                        highp float k1,
                                        highp float alpha)
{
    highp vec3  q = mix(q0, q1, alpha);
    highp float k = mix(k0, k1, alpha);
    return q / max(k, 0.000001);
}

highp float next_cell_alpha(highp vec2 pixel_start,
                            highp vec2 pixel_end,
                            highp vec2 trace_delta,
                            highp float current_alpha,
                            int mip_level)
{
    highp float cell_size     = exp2(float(mip_level));
    highp float current_major = mix(pixel_start.x, pixel_end.x, current_alpha);
    highp float next_boundary_major;

    if (trace_delta.x > 0.0)
    {
        next_boundary_major = (floor(current_major / cell_size) + 1.0) * cell_size;
    }
    else
    {
        next_boundary_major = floor((current_major - 0.0001) / cell_size) * cell_size;
    }

    highp float safe_major_delta = (trace_delta.x >= 0.0) ? max(trace_delta.x, 0.0001) : min(trace_delta.x, -0.0001);
    highp float alpha_step       = abs((next_boundary_major - current_major) / safe_major_delta);
    highp float min_step   = 0.5 * cell_size / max(abs(trace_delta.x), 0.0001);
    return min(current_alpha + max(alpha_step, min_step), 1.0);
}

bool binary_search_hit_screen(highp vec2 pixel_start,
                              highp vec2 pixel_end,
                              bool swap_axes,
                              highp vec3 q0,
                              highp vec3 q1,
                              highp float k0,
                              highp float k1,
                              highp vec3 surface_view_position,
                              highp vec2 viewport_size,
                              highp vec2 scale,
                              highp vec2 offset,
                              highp float min_alpha,
                              highp float max_alpha,
                              out highp vec2 hit_texcoord,
                              out highp vec2 hit_uv,
                              out highp float hit_distance)
{
    bool has_binary_hit = false;
    hit_texcoord        = vec2(0.0, 0.0);
    hit_uv              = vec2(0.0, 0.0);
    hit_distance        = ssr_max_distance;

    for (int binary_step = 0; binary_step < ssr_binary_search_steps; ++binary_step)
    {
        highp float binary_mid_alpha = 0.5 * (min_alpha + max_alpha);
        highp vec2  binary_pixel      = mix(pixel_start, pixel_end, binary_mid_alpha);
        highp vec2  binary_local_pixel = swap_axes ? binary_pixel.yx : binary_pixel;
        highp vec2  binary_uv         = binary_local_pixel / viewport_size;
        if (binary_uv.x < 0.0 || binary_uv.x > 1.0 || binary_uv.y < 0.0 || binary_uv.y > 1.0)
        {
            max_alpha = binary_mid_alpha;
            continue;
        }

        highp vec2 binary_texcoord     = binary_uv * scale + offset;
        highp vec3 binary_view_position = sample_segment_view_position(q0, q1, k0, k1, binary_mid_alpha);
        highp float ray_linear_depth    = linear_view_depth(binary_view_position);
        highp float scene_linear_depth  = sample_scene_linear_depth(binary_texcoord);
        if (!is_valid_scene_linear_depth(scene_linear_depth))
        {
            min_alpha = binary_mid_alpha;
            continue;
        }

        highp float depth_delta = ray_linear_depth - scene_linear_depth;
        highp float thickness   = thickness_at_depth(ray_linear_depth);
        if (depth_delta > 0.0)
        {
            max_alpha = binary_mid_alpha;
            if (depth_delta < thickness)
            {
                has_binary_hit = true;
                hit_texcoord   = binary_texcoord;
                hit_uv         = binary_uv;
                hit_distance   = length(binary_view_position - surface_view_position);
            }
        }
        else
        {
            min_alpha = binary_mid_alpha;
        }
    }

    highp float final_alpha       = 0.5 * (min_alpha + max_alpha);
    highp vec2  final_pixel       = mix(pixel_start, pixel_end, final_alpha);
    highp vec2  final_local_pixel = swap_axes ? final_pixel.yx : final_pixel;
    highp vec2  final_uv          = final_local_pixel / viewport_size;
    if (final_uv.x < 0.0 || final_uv.x > 1.0 || final_uv.y < 0.0 || final_uv.y > 1.0)
    {
        return has_binary_hit;
    }

    highp vec2  final_texcoord      = final_uv * scale + offset;
    highp vec3  final_view_position = sample_segment_view_position(q0, q1, k0, k1, final_alpha);
    highp float final_ray_depth     = linear_view_depth(final_view_position);
    highp float final_scene_depth   = sample_scene_linear_depth(final_texcoord);
    if (!is_valid_scene_linear_depth(final_scene_depth))
    {
        return has_binary_hit;
    }

    highp float final_depth_delta = final_ray_depth - final_scene_depth;
    highp float final_thickness   = thickness_at_depth(final_ray_depth);
    if (final_depth_delta > 0.0 && final_depth_delta < final_thickness)
    {
        hit_texcoord = final_texcoord;
        hit_uv       = final_uv;
        hit_distance = length(final_view_position - surface_view_position);
        return true;
    }

    return has_binary_hit;
}

void main()
{
    highp ivec2 tex_dim       = textureSize(in_scene_color, 0);
    highp vec2  viewport_size = vec2(float(width), float(height));
    highp vec2  scale         = vec2(float(width) / float(tex_dim.x), float(height) / float(tex_dim.y));
    highp vec2  offset        = vec2(float(offset_x) / float(tex_dim.x), float(offset_y) / float(tex_dim.y));
    highp vec2  texcoord      = in_texcoord * scale + offset;

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
    if (R.z >= -0.0001)
    {
        outFragColor = vec4(scene_color, 1.0);
        return;
    }

    highp vec3 origin = view_position + view_normal * ssr_ray_bias;

    highp vec2 origin_uv;
    highp vec4 origin_clip;
    if (!project_to_uv_unclamped(origin, origin_uv, origin_clip))
    {
        outFragColor = vec4(scene_color, 1.0);
        return;
    }

    highp vec3 trace_end = origin + R * ssr_max_distance;
    highp vec2 end_uv;
    highp vec4 end_clip;
    if (!project_to_uv_unclamped(trace_end, end_uv, end_clip))
    {
        outFragColor = vec4(scene_color, 1.0);
        return;
    }

    highp vec2 pixel_start = origin_uv * viewport_size;
    highp vec2 pixel_end   = end_uv * viewport_size;
    highp vec2 trace_delta = pixel_end - pixel_start;
    bool       swap_axes   = abs(trace_delta.y) > abs(trace_delta.x);
    if (swap_axes)
    {
        pixel_start = pixel_start.yx;
        pixel_end   = pixel_end.yx;
        trace_delta = trace_delta.yx;
    }

    if (abs(trace_delta.x) < 0.0001)
    {
        outFragColor = vec4(scene_color, 1.0);
        return;
    }

    highp float k0 = 1.0 / origin_clip.w;
    highp float k1 = 1.0 / end_clip.w;
    highp vec3  q0 = origin * k0;
    highp vec3  q1 = trace_end * k1;

    int start_mip =
        min(min(get_hiz_max_mip(), get_viewport_max_mip()), ssr_hiz_start_mip);
    int         current_mip         = start_mip;
    highp float current_alpha       = 0.0;
    highp float previous_alpha      = 0.0;
    highp float previous_depth_delta = -1.0;
    bool        has_previous_sample = false;
    bool        has_hit             = false;
    highp vec2  hit_texcoord        = texcoord;
    highp vec2  hit_uv              = in_texcoord;
    highp float hit_distance        = 0.0;

    for (int step_index = 0; step_index < ssr_max_steps; ++step_index)
    {
        highp float next_alpha = next_cell_alpha(pixel_start, pixel_end, trace_delta, current_alpha, current_mip);
        if (next_alpha <= current_alpha + 0.00001)
        {
            break;
        }

        highp vec2 stepped_pixel     = mix(pixel_start, pixel_end, next_alpha);
        highp vec2 local_pixel       = swap_axes ? stepped_pixel.yx : stepped_pixel;
        highp vec2 projected_uv      = local_pixel / viewport_size;
        if (projected_uv.x < 0.0 || projected_uv.x > 1.0 || projected_uv.y < 0.0 || projected_uv.y > 1.0)
        {
            break;
        }

        highp vec2 projected_texcoord = projected_uv * scale + offset;
        highp vec3 sample_position     = sample_segment_view_position(q0, q1, k0, k1, next_alpha);
        highp float distance_to_sample = length(sample_position - view_position);
        if (distance_to_sample > ssr_max_distance)
        {
            break;
        }

        highp float ray_linear_depth = linear_view_depth(sample_position);
        highp float thickness        = thickness_at_depth(ray_linear_depth);
        highp float hiz_linear_depth = sample_hiz_linear_depth(projected_texcoord, current_mip);

        if (ray_linear_depth + thickness < hiz_linear_depth)
        {
            current_alpha = next_alpha;
            if (current_mip < start_mip)
            {
                current_mip += 1;
            }
            continue;
        }

        if (current_mip > 0)
        {
            current_mip -= 1;
            continue;
        }

        highp float scene_linear_depth = sample_scene_linear_depth(projected_texcoord);
        if (!is_valid_scene_linear_depth(scene_linear_depth))
        {
            current_alpha = next_alpha;
            continue;
        }

        highp float depth_delta = ray_linear_depth - scene_linear_depth;
        if (depth_delta > 0.0 && depth_delta < thickness)
        {
            has_hit      = true;
            hit_texcoord = projected_texcoord;
            hit_uv       = projected_uv;
            hit_distance = distance_to_sample;
            break;
        }

        if (has_previous_sample && previous_depth_delta < 0.0 && depth_delta > 0.0 &&
            binary_search_hit_screen(pixel_start,
                                     pixel_end,
                                     swap_axes,
                                     q0,
                                     q1,
                                     k0,
                                     k1,
                                     view_position,
                                     viewport_size,
                                     scale,
                                     offset,
                                     previous_alpha,
                                     next_alpha,
                                     hit_texcoord,
                                     hit_uv,
                                     hit_distance))
        {
            has_hit = true;
            break;
        }

        has_previous_sample  = true;
        previous_alpha       = next_alpha;
        previous_depth_delta = depth_delta;
        current_alpha        = next_alpha;
        if (current_mip < start_mip)
        {
            current_mip += 1;
        }
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

    outFragColor = vec4(mix(scene_color, reflection_color, reflection_alpha), 1.0);
}
