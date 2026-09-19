#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	vec2 size;
	vec2 gradientDir;
	vec4 colorA;
	vec4 colorB;
	vec4 borderColor;
	vec4 cornerRadii;
	vec4 clipRect;
	float gradientExponent;
	float borderWidth;
	float edgeSoftness;
};

layout(location = 0) in vec4 vertex;

layout(location = 0) out vec2 uv;

void main()
{
	gl_Position = transformationMatrix * vertex;
	// The shared rect buffer is a unit quad - vertex.xy doubles as 0..1 uv.
	uv = vertex.xy;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
