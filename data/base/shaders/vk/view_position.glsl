#ifndef WZ_VIEW_POSITION_GLSL
#define WZ_VIEW_POSITION_GLSL

// `uv` is the logical screen position in [0, 1] over the pass's write viewport
// (texCoords of the fullscreen triangle, or a projected NDC mapped to 0..1).
// It is NOT a texel coordinate: with dynamic resolution the scene is rendered
// into a sub-rectangle of a native-sized allocation, so texel = uv * uvScaleClamp.xy.
// Feeding the scaled texel coordinate here reconstructs x/y at the wrong place.
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
