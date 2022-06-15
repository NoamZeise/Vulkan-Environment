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

layout(set = 1, binding = 0) readonly buffer PerInstanceData
{
    mat4 model;
    mat4 normalMat;
} pid[100];

const int MAX_BONES = 50;
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
    for(int i = 0; i < 4; i++)
    {
      if(inBoneIDs[i] == -1 || inBoneIDs[i] >= MAX_BONES)
          break;
      skin += inWeights[i] * bones.mat[inBoneIDs[i]];
    }

    vec4 fragPos = vec4(0.0);
    if(pcs.normalMat[3][3] == 0.0) //draw instance (use per frame buffer)
    {
        fragPos = ubo.view * pid[gl_InstanceIndex].model * skin * vec4(inPos, 1.0f);
        outNormal = (pid[gl_InstanceIndex].normalMat * skin * vec4(inNormal, 1.0f)).xyz;
    }
    else //draw once (use push constants)
    {
        fragPos = ubo.view * pcs.model * skin * vec4(inPos, 1.0f);
        outNormal = (pcs.normalMat * skin * vec4(inNormal, 1.0f)).xyz;
    }

    gl_Position = ubo.proj * fragPos;
    outFragPos = vec3(fragPos) / fragPos.w;
}
