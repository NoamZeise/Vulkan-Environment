#version 450

layout(push_constant) uniform vertconstants
{
    mat4 model;
} pcs;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
} ubo;


layout(location = 0) in vec3 inPos;
layout(loaction = 1) in vec3 inNormal;
layout(location = 2) in vec2 intexCoord;

layout(location = 0) out vec2 texCoord;

void main()
{
    gl_Position = ubo.proj * ubo.view * pcs.model * vec4(inPos, 0, 1);
    texCoord = intexCoord;
}
