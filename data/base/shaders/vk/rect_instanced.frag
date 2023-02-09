#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ProjectionMatrix;
};

layout(location = 1) in vec4 colour;

layout(location = 0) out vec4 FragColor;

void main()
{
	FragColor = colour;
}
