#version 310 es

layout (location = 0) in vec3 aPos;

layout(set = 0, binding = 0) uniform UBO
{
    mat4 proj_view;
    mat4 model;
} ubo;

void main()
{
    gl_Position = ubo.proj_view * ubo.model * vec4(aPos, 1.0);
}