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

layout(set = 1, binding = 0) readonly buffer PerInstanceData {
    mat4 model;
    mat4 normalMat;
} pid[100];


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outFragPos;
layout(location = 2) out vec3 outNormal;



void main()
{
    outTexCoord = inTexCoord;
    vec4 fragPos = vec4(0.0);
    if(pcs.normalMat[3][3] == 0.0) //draw instance (use per frame buffer)
    {
        fragPos = ubo.view * pid[gl_InstanceIndex].model * vec4(inPos, 1.0);
        outNormal = mat3(pid[gl_InstanceIndex].normalMat) * inNormal;
    }
    else //draw once (use push constants)
    {
        fragPos = ubo.view * pcs.model * vec4(inPos, 1.0);
        outNormal = mat3(pcs.normalMat) * inNormal;
    }
    gl_Position = ubo.proj * fragPos;
    outFragPos = vec3(fragPos) / fragPos.w;
}
