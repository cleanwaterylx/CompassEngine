#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(set = 0, binding = 0) uniform sampler2D in_ssao;
layout(set = 0, binding = 1) readonly buffer ssao_kernel_buffer
{
    highp mat4 proj_matrix;
    highp mat4 view_matrix;
    highp vec4 ssao_kernel[ssao_kernel_size];
    int offset_x;
    int offset_y;
    int width;
    int height;
};

layout(location = 0) in highp vec2 in_texcoord;

layout(location = 0) out highp vec4 outFragColor;

void main()
{
    highp ivec2 texDim = textureSize(in_ssao, 0);
    highp vec2 scale = vec2(float(width)/float(texDim.x), float(height)/float(texDim.y));
    highp vec2 offset = vec2(float(offset_x)/float(texDim.x), float(offset_y)/float(texDim.y));
    highp vec2 texcoord = in_texcoord * scale;
    texcoord += offset;       // real texcoord

    const int blurRange = 2;
    int n = 0;
    highp vec2 texelSize = 1.0 / vec2(width, height);
    highp float result = 0.0;
    for (int x = -blurRange; x < blurRange; x++)
    {
        for (int y = -blurRange; y < blurRange; y++)
        {
            highp vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(in_ssao, texcoord + offset).r;
            n++;
        }
    }
    outFragColor.x = result / (float(n));
}