#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ViewMatrix;
};

layout(location = 0) in vec4 vertex;

layout(location = 0) out vec3 viewNormal;

void main()
{
	gl_Position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.0);
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	viewNormal = mat3(ViewMatrix) * vec3(0.0, 1.0, 0.0);
}
