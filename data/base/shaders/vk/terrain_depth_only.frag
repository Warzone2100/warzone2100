#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
};

void main()
{
//	FragColor = fragColor;
}
