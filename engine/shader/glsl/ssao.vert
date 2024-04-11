#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(location = 0) out vec2 out_texcoord;

void main()
{
    // 渲染全屏四边形（或填充整个屏幕的东西）
    // 因为许多效果依赖于在 [0..1] 范围内使用适当的 uv 坐标在整个屏幕上渲染纹理。这适用于后期处理（发光、模糊、ssao）、延迟着色或程序生成的输出。

    vec3 fullscreen_triangle_positions[3] = vec3[3](vec3(3.0, 1.0, 0.5), vec3(-1.0, 1.0, 0.5), vec3(-1.0, -3.0, 0.5));

    vec2 fullscreen_triangle_uvs[3] = vec2[3](vec2(2.0, 1.0), vec2(0.0, 1.0), vec2(0.0, -1.0));

    gl_Position  = vec4(fullscreen_triangle_positions[gl_VertexIndex], 1.0);
    out_texcoord = fullscreen_triangle_uvs[gl_VertexIndex];
}