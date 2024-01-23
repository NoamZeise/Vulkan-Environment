#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
} ubo;

struct Obj3DPerFrame
{
    mat4 model;
    mat4 normalMat_M;
};

layout(std140, set = 1, binding = 0) readonly buffer PerInstanceData
{
    Obj3DPerFrame data[];
} pid;


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outFragPos_world;
layout(location = 2) out vec3 outNormal_world;



void main()
{
    outTexCoord = inTexCoord;
    vec4 fragPos = pid.data[gl_InstanceIndex].model * vec4(inPos, 1.0);
    outNormal_world = vec3(pid.data[gl_InstanceIndex].normalMat_M * vec4(inNormal, 0.0));

    gl_Position = ubo.proj * ubo.view * fragPos;
    outFragPos_world = vec3(fragPos) / fragPos.w;
}
