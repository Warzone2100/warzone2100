#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 orthoViewProj;
	vec4 mapOriginExtent; // xy origin.xz, zw size.xz
	vec4 sdfParams; // x = sdfBand
};

layout(location = 0) in vec4 vertex; // xyz unit cone
layout(location = 9) in vec4 instancePackedValues; // xy = render XZ center, z = radius

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec3 centerRadius; // xy center, z R

void main()
{
	// Unit cone (apex y=0.8, rim y=-0.2 at radius 1.25) scaled by R. Under the
	// top-down ortho, side triangles fill a 1.25R disk: conservative coverage
	// of texels that might be near the ring (true isocontour is at distance R).
	// Y falls 0.8 per unit of planar distance for every instance, so the
	// interpolated depth is 0.8 * (r - R) plus a shared offset and the GPU LEQ
	// union keeps the instance with the smallest true signed distance.
	vec2 center = instancePackedValues.xy;
	float R = instancePackedValues.z;
	vec3 wp = vec3(center.x, 0.0, center.y) + vec3(vertex.x * R, vertex.y * R, vertex.z * R);
	worldPos = wp;
	centerRadius = vec3(center, R);
	gl_Position = orthoViewProj * vec4(wp, 1.0);
	// Negate clip Y so world minZ lands at V=0, matching the composite's
	// (world.xz - origin) / extent map.
	gl_Position.y *= -1.;
	// GL-style clip Z is [-w, w]; Vulkan NDC Z is [0, 1].
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
