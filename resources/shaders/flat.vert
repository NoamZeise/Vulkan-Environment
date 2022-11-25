#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
} ubo;


layout(set = 1, binding = 0) readonly buffer PerFrameBuffer {
    mat4 model[];
} pid;


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec3 outTexCoord;

void main()
{
    outTexCoord = vec3(inTexCoord.xy, gl_InstanceIndex);
    vec4 fragPos = ubo.view * pid.model[gl_InstanceIndex] * vec4(inPos, 1.0);

    gl_Position = ubo.proj * fragPos;
}
