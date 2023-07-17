// common functions shared by terrainDecails.frag shaders

float getShadowMapDepthComp(vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, int cascadeIndex, float z)
{
	vec2 uv = base_uv + vec2(u, v) * shadowMapSizeInv;
	return texture( shadowMap, vec4(uv, cascadeIndex, z) );
}

float getShadowVisibility()
{
#if WZ_SHADOW_MODE == 0 || WZ_SHADOW_FILTER_SIZE == 0
	// no shadow-mapping
	return 1.0;
#else
	// Shadow Mapping

	vec4 fragPosViewSpace = ViewMatrix * vec4(fragPos, 1.0);
	float depthValue = abs(fragPosViewSpace.z);

	int cascadeIndex = 0;
//	for (int i = 0; i < WZ_SHADOW_CASCADES_COUNT - 1; ++i)
//	{
//		if (depthValue >= ShadowMapCascadeSplits[i])
//		{
//			cascadeIndex = i + 1;
//		}
//	}
	// unrolled loop, using vec4 swizzles
#if WZ_SHADOW_CASCADES_COUNT > 1
	if (depthValue >= ShadowMapCascadeSplits.x)
	{
		cascadeIndex = 1;
	}
#endif
#if WZ_SHADOW_CASCADES_COUNT > 2
	if (depthValue >= ShadowMapCascadeSplits.y)
	{
		cascadeIndex = 2;
	}
#endif
#if WZ_SHADOW_CASCADES_COUNT > 3
	if (depthValue >= ShadowMapCascadeSplits.z)
	{
		cascadeIndex = 3;
	}
#endif

	vec4 shadowPos = ShadowMapMVPMatrix[cascadeIndex] * vec4(fragPos, 1.0);
	vec3 pos = shadowPos.xyz / shadowPos.w;

	if (pos.z > 1.0f)
	{
		return 1.0;
	}

	float bias = 0.0002f;

#if WZ_SHADOW_MODE == 1

	// Optimized PCF algorithm
	// See: https://therealmjp.github.io/posts/shadow-maps/
	// And: http://www.ludicon.com/castano/blog/articles/shadow-mapping-summary-part-1/

	vec2 shadowMapSize = vec2(float(ShadowMapSize), float(ShadowMapSize));

	float visibility = 0.f;

	float lightDepth = pos.z;

	lightDepth += bias;

	vec2 uv = pos.xy * shadowMapSize; // 1 unit - 1 texel

	vec2 shadowMapSizeInv = 1.0 / shadowMapSize;

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	float sum = 0.0;

#if WZ_SHADOW_FILTER_SIZE <= 3
	// 3x3

	float uw0 = (3.0 - 2.0 * s);
	float uw1 = (1.0 + 2.0 * s);

	float u0 = (2.0 - s) / uw0 - 1.0;
	float u1 = s / uw1 + 1.0;

	float vw0 = (3.0 - 2.0 * t);
	float vw1 = (1.0 + 2.0 * t);

	float v0 = (2.0 - t) / vw0 - 1.0;
	float v1 = t / vw1 + 1.0;

	sum += uw0 * vw0 * getShadowMapDepthComp(base_uv, u0, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw0 * getShadowMapDepthComp(base_uv, u1, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw0 * vw1 * getShadowMapDepthComp(base_uv, u0, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw1 * getShadowMapDepthComp(base_uv, u1, v1, shadowMapSizeInv, cascadeIndex, lightDepth);

	visibility = sum * 1.0 / 16.0;

#elif WZ_SHADOW_FILTER_SIZE > 3 && WZ_SHADOW_FILTER_SIZE <= 5
	// 5x5

	float uw0 = (4.0 - 3.0 * s);
	float uw1 = 7.0;
	float uw2 = (1.0 + 3.0 * s);

	float u0 = (3.0 - 2.0 * s) / uw0 - 2.0;
	float u1 = (3.0 + s) / uw1;
	float u2 = s / uw2 + 2.0;

	float vw0 = (4.0 - 3.0 * t);
	float vw1 = 7.0;
	float vw2 = (1.0 + 3.0 * t);

	float v0 = (3.0 - 2.0 * t) / vw0 - 2.0;
	float v1 = (3.0 + t) / vw1;
	float v2 = t / vw2 + 2.0;

	sum += uw0 * vw0 * getShadowMapDepthComp(base_uv, u0, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw0 * getShadowMapDepthComp(base_uv, u1, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw0 * getShadowMapDepthComp(base_uv, u2, v0, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw1 * getShadowMapDepthComp(base_uv, u0, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw1 * getShadowMapDepthComp(base_uv, u1, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw1 * getShadowMapDepthComp(base_uv, u2, v1, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw2 * getShadowMapDepthComp(base_uv, u0, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw2 * getShadowMapDepthComp(base_uv, u1, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw2 * getShadowMapDepthComp(base_uv, u2, v2, shadowMapSizeInv, cascadeIndex, lightDepth);

	visibility = sum * 1.0 / 144.0;

#else
	// WZ_SHADOW_FILTER_SIZE > 5
	// 7x7

	float uw0 = (5.0 * s - 6.0);
	float uw1 = (11.0 * s - 28.0);
	float uw2 = -(11.0 * s + 17.0);
	float uw3 = -(5.0 * s + 1.0);

	float u0 = (4.0 * s - 5.0) / uw0 - 3.0;
	float u1 = (4.0 * s - 16.0) / uw1 - 1.0;
	float u2 = -(7.0 * s + 5.0) / uw2 + 1.0;
	float u3 = -s / uw3 + 3.0;

	float vw0 = (5.0 * t - 6.0);
	float vw1 = (11.0 * t - 28.0);
	float vw2 = -(11.0 * t + 17.0);
	float vw3 = -(5.0 * t + 1.0);

	float v0 = (4.0 * t - 5.0) / vw0 - 3.0;
	float v1 = (4.0 * t - 16.0) / vw1 - 1.0;
	float v2 = -(7.0 * t + 5.0) / vw2 + 1.0;
	float v3 = -t / vw3 + 3.0;

	sum += uw0 * vw0 * getShadowMapDepthComp(base_uv, u0, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw0 * getShadowMapDepthComp(base_uv, u1, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw0 * getShadowMapDepthComp(base_uv, u2, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw3 * vw0 * getShadowMapDepthComp(base_uv, u3, v0, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw1 * getShadowMapDepthComp(base_uv, u0, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw1 * getShadowMapDepthComp(base_uv, u1, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw1 * getShadowMapDepthComp(base_uv, u2, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw3 * vw1 * getShadowMapDepthComp(base_uv, u3, v1, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw2 * getShadowMapDepthComp(base_uv, u0, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw2 * getShadowMapDepthComp(base_uv, u1, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw2 * getShadowMapDepthComp(base_uv, u2, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw3 * vw2 * getShadowMapDepthComp(base_uv, u3, v2, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw3 * getShadowMapDepthComp(base_uv, u0, v3, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw3 * getShadowMapDepthComp(base_uv, u1, v3, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw3 * getShadowMapDepthComp(base_uv, u2, v3, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw3 * vw3 * getShadowMapDepthComp(base_uv, u3, v3, shadowMapSizeInv, cascadeIndex, lightDepth);

	visibility = sum * 1.0 / 2704.0;

#endif
// end WZ_SHADOW_FILTER_SIZE checks

#endif
// end WZ_SHADOW_MODE == 1

#if WZ_SHADOW_MODE == 2

	// PCF

	float visibility = texture( shadowMap, vec4(pos.xy, cascadeIndex, (pos.z+bias)) );

#if WZ_SHADOW_FILTER_SIZE >= 2
	const float edgeVal = 0.5+float((WZ_SHADOW_FILTER_SIZE-2)/2);
	const float startVal = -edgeVal;
	const float endVal = edgeVal + 0.5;
	float texelIncrement = 1.0/float(ShadowMapSize);
	const float visibilityIncrement = 0.5 / WZ_SHADOW_FILTER_SIZE;
	for (float y=startVal; y<endVal; y+=1.0)
	{
		for (float x=startVal; x<endVal; x+=1.0)
		{
			visibility -= visibilityIncrement*(1.0-texture( shadowMap, vec4(pos.xy + vec2(x*texelIncrement, y*texelIncrement), cascadeIndex, (pos.z+bias)) ));
		}
	}
#endif
// end WZ_SHADOW_FILTER_SIZE >= 2

#endif
// end WZ_SHADOW_MODE == 2

	visibility = clamp(visibility, 0.3, 1.0);
	return visibility;
#endif
}
