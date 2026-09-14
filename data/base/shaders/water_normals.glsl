#ifndef WZ_WATER_NORMALS_GLSL
#define WZ_WATER_NORMALS_GLSL

// Model-space shading normal, y-up. Matches terrain_water_high.frag.
vec3 wzWaterModelNormal(sampler2DArray tex_nm, vec2 uv1, vec2 uv2, float timeSec, float mipBias)
{
	vec3 N1 = texture2DArray(tex_nm, vec3(vec2(uv1.x, uv1.y+timeSec*0.04), 0.0), mipBias).xzy * vec3( 2.0, 2.0, 2.0) + vec3(-1.0, 0.0, -1.0);
	vec3 N2 = texture2DArray(tex_nm, vec3(vec2(uv2.x+timeSec*0.02, uv2.y), 1.0), mipBias).xzy * vec3(-2.0, 2.0,-2.0) + vec3( 1.0, -1.0, 1.0);
	vec3 N3 = texture2DArray(tex_nm, vec3(uv1.x+timeSec*0.05, uv1.y, 0.0), mipBias).xzy * 2.0 - 1.0;
	vec3 N4 = texture2DArray(tex_nm, vec3(uv2.x, uv2.y+timeSec*0.03, 1.0), mipBias).xzy * 2.0 - 1.0;
	//use RNM blending to mix normal maps properly, see https://blog.selfshadow.com/publications/blending-in-detail/
	vec3 RNM = normalize(N1 * dot(N1,N2) - N2*N1.y);
	vec3 Na = mix(N3, N4, 0.5);
	vec3 N = mix(RNM, Na, 0.5);
	return normalize(vec3(N.x, N.y * 5.0, N.z));
}

#endif
