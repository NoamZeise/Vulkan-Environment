#version 450

layout(push_constant) uniform vertconstants
{
    mat4 model;
    mat4 normalMat;
} pcs;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
} ubo;


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outVertPos;
layout(location = 2) out vec3 outNormal;

void main()
{
    gl_Position = ubo.proj * ubo.view * pcs.model * vec4(inPos, 1);
    outTexCoord = inTexCoord;
    outVertPos = (ubo.view * pcs.model * vec4(inPos, 1.0)).xyz;
    outNormal = vec3(pcs.normalMat * vec4(inNormal, 0.0));
}
