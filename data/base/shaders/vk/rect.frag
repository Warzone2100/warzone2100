#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	vec2 tuv_offset;
	vec2 tuv_scale;
	vec4 color;
};

layout(location = 0) out vec4 FragColor;

void main()
{
	FragColor = color;
}
