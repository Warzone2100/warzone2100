#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
};

layout(location = 0) in vec4 vertex;
layout(location = 3) in vec3 vertexNormal;

layout(location = 0) out vec3 viewNormal;

void main()
{
	vec4 position = ProjectionMatrix * ViewMatrix * vertex;
	gl_Position = position;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;

	viewNormal = mat3(ViewMatrix) * vertexNormal;
}
