#version 450

layout (location = 0) out vec2 outUV;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 screenTransform;
} ubo;

const vec2 pos[6] = {
{-1.0f, -1.0f},
{1.0f, -1.0f},
{1.0f,1.0f},
{1.0f, 1.0f},
{-1.0f, 1.0f},
{-1.0f, -1.0f},
};
const vec2 uv[6] = {
{0.0f, 0.0f},
{1.0f, 0.0f},
{1.0f,1.0f},
{1.0f, 1.0f},
{0.0f, 1.0f},
{0.0f, 0.0f},
};

void main() 
{
	gl_Position = ubo.screenTransform * vec4(pos[gl_VertexIndex], 0.0f, 1.0f);

	outUV = uv[gl_VertexIndex];
}