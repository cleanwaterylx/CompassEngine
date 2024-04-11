#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"
#include "gbuffer.h"


// no subpassinput 
layout(set = 0, binding = 0) uniform sampler2D in_position_depth;
layout(set = 0, binding = 1) uniform sampler2D in_normal;
layout(set = 0, binding = 2) uniform sampler2D ssao_noise;

// ×¢ÒâÄÚ´æ¶ÔÆë
layout(set = 0, binding = 3) readonly buffer ssao_kernel_buffer
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
    highp ivec2 texDim = textureSize(in_position_depth, 0);
    highp ivec2 noiseDim = textureSize(ssao_noise, 0);
    highp vec2 scale = vec2(float(width)/float(texDim.x), float(height)/float(texDim.y));
    highp vec2 offset = vec2(float(offset_x)/float(texDim.x), float(offset_y)/float(texDim.y));
    highp vec2 texcoord = in_texcoord * scale;
    texcoord += offset;       // real texcoord

    // pos
    highp vec3 fragPos = texture(in_position_depth, texcoord).rgb;
    // normal
    highp vec3 normal = normalize(texture(in_normal, texcoord).rgb);

    // noise
    highp vec2 noiseUV = vec2(float(width)/float(noiseDim.x), float(height)/float(noiseDim.y)) * texcoord;
    highp vec3 randomVec = normalize(texture(ssao_noise, noiseUV).xyz);

    highp vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    highp vec3 bitangent = cross(tangent, normal);
    highp mat3 TBN = mat3(tangent, bitangent, normal);

    highp float occlusion = 0.0f;
    const highp float bias = 0.01f;
    highp vec3 samplePos;
    for(int i = 0; i < ssao_kernel_size; i++)
    {
        samplePos = TBN * ssao_kernel[i].xyz;
        samplePos = fragPos + samplePos * ssao_radius;

        highp vec4 offset_pos = vec4(samplePos, 1.0f);
        offset_pos = proj_matrix * offset_pos;
        offset_pos.xyz /= offset_pos.w;
        offset_pos.xyz = offset_pos.xyz * 0.5f + 0.5f;
        highp float sampleDepth = texture(in_position_depth, offset_pos.xy * scale + offset).z;

        highp float rangeCheck = smoothstep(0.0, 1.0, ssao_radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck; 
        // occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0); 
    }

    occlusion = 1.0 - (occlusion / float(ssao_kernel_size));
    outFragColor.x = occlusion;
}




