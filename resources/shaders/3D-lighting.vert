#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
    vec4 lightPos;
} ubo;

struct Obj3DPerFrame
{
    mat4 model;
    mat4 normalMat;
};

layout(std140, set = 1, binding = 0) readonly buffer PerInstanceData
{
    Obj3DPerFrame data[];
} pid;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outFragPos_world;
layout(location = 2) out vec3 outLightDir_cam;
layout(location = 3) out vec3 outEyeDir_cam;
layout(location = 4) out vec3 outLightDir_tangent;
layout(location = 5) out vec3 outEyeDir_tangent;


void main()
{
    outTexCoord = inTexCoord;
    
    outFragPos_world = (pid.data[gl_InstanceIndex].model * vec4(inPos, 1.0)).xyz;
    vec4 fragCamPos = ubo.view * pid.data[gl_InstanceIndex].model * vec4(inPos, 1.0);
    outEyeDir_cam = vec3(0)-fragCamPos.xyz;

    vec3 lightPos = (ubo.view * ubo.lightPos).xyz;
    outLightDir_cam = lightPos + outEyeDir_cam;

    mat3 nm = mat3(pid.data[gl_InstanceIndex].normalMat);
    mat3 tbn = transpose(mat3(
    nm * inTangent,
    nm * inBitangent,
    nm * inNormal));

    outLightDir_tangent = tbn * outLightDir_cam;
    outEyeDir_tangent = tbn * outEyeDir_cam;
    
    gl_Position = ubo.proj * fragCamPos;
}
