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

void main()
{
    highp ivec2 tex_dim = textureSize(in_scene_color, 0);
    highp vec2  scale   = vec2(float(width) / float(tex_dim.x), float(height) / float(tex_dim.y));
    highp vec2  offset  = vec2(float(offset_x) / float(tex_dim.x), float(offset_y) / float(tex_dim.y));
    highp vec2  texcoord = in_texcoord * scale + offset;

    outFragColor = texture(in_scene_color, texcoord);
}
