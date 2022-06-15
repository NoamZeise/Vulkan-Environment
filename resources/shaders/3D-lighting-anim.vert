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

layout(set = 2, binding = 0) uniform boneView
{
 int viewBoneID;
} bvu;


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

    vec3 boneColour = vec3(0.0, 1.0, 1.0);
    for(int i = 0; i < 4; i++)
    {
        if(inBoneIDs[i] == bvu.viewBoneID && inBoneIDs[i] != -1)
        {
            if(inWeights[i] >= 0.7)
                boneColour = vec3(1.0, 0.0, 0.0) * inWeights[i];
            else if(inWeights[i] >= 0.4)
                boneColour = vec3(0.0, 1.0, 0.0) *  inWeights[i];
            else
                boneColour = vec3(1.0, 1.0, 0.0) * inWeights[i];

        }
    }
    outBoneColour = boneColour;

    gl_Position = ubo.proj * fragPos;
    outFragPos = vec3(fragPos) / fragPos.w;
}
