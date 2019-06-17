#version 450

layout(location = 1) in vec4 vColour;
layout(location = 0) out vec4 FragColor;

void main()
{
	FragColor = vColour;
}
