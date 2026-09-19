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

#endif
