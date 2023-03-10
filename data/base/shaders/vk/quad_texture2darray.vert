#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	mat4 uvTransformMatrix;
	ivec4 swizzle;
	vec4 color;
	int layer;
};

layout(location = 0) in vec4 vertex;
layout(location = 0) out vec2 uv;

void main()
{
	gl_Position = transformationMatrix * vertex;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	vec4 calculatedCoord = uvTransformMatrix * vec4(vertex.xy, 1.f, 1.f);
	uv = calculatedCoord.xy;
}
