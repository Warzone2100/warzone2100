#version 450

layout(set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	vec4 color;
};

layout(location = 0)in vec4 vertex;

void main()
{
	gl_Position = ModelViewProjectionMatrix * vertex;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}