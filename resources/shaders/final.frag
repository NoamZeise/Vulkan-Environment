#version 450

//from https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/offscreen

layout(set = 0, binding = 0) uniform sampler offscreenSampler;
layout(set = 0, binding = 1) uniform texture2D offscreenTex;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = texture(sampler2D(offscreenTex, offscreenSampler), inUV);
}