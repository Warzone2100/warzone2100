#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
};

layout(location = 0) in vec4 vertex;

void main()
{
	vec4 position = ModelViewProjectionMatrix * vertex;
	position.z += 0.01f;
	gl_Position = position;

//	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
