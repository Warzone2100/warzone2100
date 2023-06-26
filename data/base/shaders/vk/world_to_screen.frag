#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	float gamma;
};

layout(set = 1, binding = 0) uniform sampler2D Texture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec3 texColour = texture(Texture, texCoords).rgb;

	FragColor = vec4(texColour, 1.0);
}
