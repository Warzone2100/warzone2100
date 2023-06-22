#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	float gamma;
};

layout(location = 0) in vec2 vertexPos;
layout(location = 1) in vec2 vertexTexCoords;

layout(location = 0) out vec2 texCoords;

void main()
{
	texCoords = vertexTexCoords;
	gl_Position = vec4(vertexPos.x, vertexPos.y, 0.f, 1.f);
//	gl_Position.y *= -1.;
//	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
