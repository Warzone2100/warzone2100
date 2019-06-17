#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	vec2 tuv_offset;
	vec2 tuv_scale;
	vec4 color;
};
layout(set = 1, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 FragColor;

void main()
{
	FragColor = texture(tex, uv) * color;
}
