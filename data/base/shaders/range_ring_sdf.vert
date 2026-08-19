// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

layout(std140) uniform cbuffer {
	mat4 orthoViewProj;
	vec4 mapOriginExtent; // xy origin.xz, zw size.xz
	vec4 sdfParams; // x = sdfBand
};

#ifdef NEWGL
#define VERTEX_INPUT in
#define VERTEX_OUTPUT out
#else
#define VERTEX_INPUT attribute
#define VERTEX_OUTPUT varying
#endif

VERTEX_INPUT vec4 vertex; // xyz unit cone
VERTEX_INPUT vec4 instancePackedValues; // xy = render XZ center, z = radius

VERTEX_OUTPUT vec3 worldPos;
VERTEX_OUTPUT vec3 centerRadius; // xy center, z R

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
	// OpenGL FBOs have UV origin at the bottom-left. Negate clip Y so world minZ
	// lands at V=0, matching the composite's (world.xz - origin) / extent map
	// (Vulkan already gets this from its NDC Y flip).
	gl_Position.y *= -1.0;
}
