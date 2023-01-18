#version 450

layout (location = 0) out vec2 outUV;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 screenTransform;
} ubo;

void main() 
{
//this method from https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/offscreen/quad.vert
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = ubo.screenTransform * vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}