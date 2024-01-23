#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
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

const int MAX_BONES = 80;
layout(set = 2, binding = 0) uniform boneView
{
   mat4 mat[MAX_BONES];
} bones;


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in ivec4 inBoneIDs;
layout(location = 4) in vec4 inWeights;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outFragPos;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outBoneColour;

void main()
{
    outTexCoord = inTexCoord;

    mat4 skin = mat4(0.0f);
    for(int i = 0; i < 4; i++) {
      skin += inWeights[i] * bones.mat[inBoneIDs[i]];
    }

    vec4 fragPos = pid.data[gl_InstanceIndex].model * skin * vec4(inPos, 1.0f);
    outNormal = mat3(pid.data[gl_InstanceIndex].normalMat) * mat3(skin) * inNormal;

    gl_Position = ubo.proj * ubo.view * fragPos;
    outFragPos = vec3(fragPos) / fragPos.w;
}
