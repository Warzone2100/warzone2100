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

// View Z from depth when clip.z is independent of x/y (pie_PerspectiveGet:
// frustum * scale(1,1,-1), then xy translate). projZCoeffs = (P[2][2], P[3][2]).
float wzGetViewZ(float depth, vec2 projZCoeffs)
{
	float ndcZ = depth * 2.0 - 1.0;
	return projZCoeffs.y / (ndcZ - projZCoeffs.x);
}

#endif
