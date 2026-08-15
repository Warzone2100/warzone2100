#ifndef WZ_VIEW_POSITION_GLSL
#define WZ_VIEW_POSITION_GLSL

vec3 wzGetViewPosition(vec2 uv, float depth, mat4 invProjectionMatrix)
{
	// Depth prepass vertices apply gl_Position.y *= -1 for Vulkan NDC; pie_PerspectiveGet does not.
	vec2 clipXY = uv * 2.0 - 1.0;
	clipXY.y = -clipXY.y;
	// Stored depth is [0, 1] (same mapping as OpenGL after the prepass z remap).
	float clipZ = depth * 2.0 - 1.0;
	vec4 clipSpace = vec4(clipXY, clipZ, 1.0);
	vec4 viewSpace = invProjectionMatrix * clipSpace;
	return viewSpace.xyz / viewSpace.w;
}

#endif
