#ifndef WZ_VIEW_POSITION_GLSL
#define WZ_VIEW_POSITION_GLSL

vec3 wzGetViewPosition(vec2 uv, float depth, mat4 invProjectionMatrix)
{
	// OpenGL depth buffer stores NDC Z mapped from [-1, 1] to [0, 1].
	float clipZ = depth * 2.0 - 1.0;
	vec4 clipSpace = vec4(uv * 2.0 - 1.0, clipZ, 1.0);
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
