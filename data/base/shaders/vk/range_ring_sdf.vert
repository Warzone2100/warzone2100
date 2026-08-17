#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 orthoViewProj;
	vec4 mapOriginExtent; // xy origin.xz, zw size.xz
	vec4 sdfParams; // x = sdfBand
};

layout(location = 0) in vec4 vertex;
layout(location = 9) in vec4 instancePackedValues; // xy = render XZ center, z = radius

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec3 centerRadius; // xy center, z R

void main()
{
	vec2 center = instancePackedValues.xy;
	float R = instancePackedValues.z;
	vec3 wp = vec3(center.x, 0.0, center.y) + vec3(vertex.x * R, vertex.y * R, vertex.z * R);
	worldPos = wp;
	centerRadius = vec3(center, R);
	gl_Position = orthoViewProj * vec4(wp, 1.0);
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
