// Ray-traced point-light shadow: 1.0 = unoccluded, 0.0 = shadowed.
// Only available in the ray-query shader variants (requires the wzTopLevelAS declaration).
float wzPointLightOcclusion(vec3 fragWorldPos, vec3 lightWorldPos)
{
#if WZ_RAYQUERY_SHADERS
	vec3 v = lightWorldPos - fragWorldPos;
	float dist = length(v);
	if (dist < 16.0)
	{
		return 1.0; // fragment is essentially at the light
	}
	vec3 dir = v / dist;
	rayQueryEXT rayQuery;
	// offset the origin along the ray (we may not have a world-space normal here), and stop
	// short of the light so emitter geometry (muzzles, lamps) doesn't shadow its own light
	rayQueryInitializeEXT(rayQuery, wzTopLevelAS,
						  gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
						  0xFF, fragWorldPos + dir * 4.0, 0.0, dir, max(dist - 24.0, 0.0));
	rayQueryProceedEXT(rayQuery);
	return (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT) ? 1.0 : 0.0;
#else
	return 1.0;
#endif
}

// See https://lisyarus.github.io/blog/graphics/2022/07/30/point-light-attenuation.html for explanation
// we want something that looks somewhat physically correct, but must absolutely be 0 past range
float pointLightEnergyAtPosition(vec3 pointLightVector, float range)
{
	float normalizedDistance = length(pointLightVector) / range; //to-do: make range based on effect size
	float sqNormDist =  normalizedDistance * normalizedDistance;
	float numerator = max(1 - sqNormDist, 0);
	return numerator * numerator / ( 1 + 2 * sqNormDist);
}

vec4 processPointLight(vec3 WorldFragPos, vec3 fragNormal, vec3 viewVector, vec4 albedo, float gloss, vec3 pointLightWorldPosition, float pointLightEnergy, vec3 pointLightColor, mat3 spaceMatrix)
{
	vec3 pointLightVector = pointLightWorldPosition - WorldFragPos;
	vec3 pointLightDir = spaceMatrix * normalize(pointLightVector);

	float energy = pointLightEnergyAtPosition(pointLightVector, pointLightEnergy);
	vec4 lightColor = vec4(pointLightColor * energy, 1.f); //to-do: pick average color from effect's texture
	//lightColor.rgb = mix(lightColor.rgb, (lightColor.rgb + 1.5)*energy*2, energy);

	float pointLightLambert = max(dot(fragNormal, pointLightDir), 0.0);
	vec3 pointLightHalfVec = normalize(pointLightDir + viewVector);
	float pointLightBlinn = clamp(pow(dot(fragNormal, pointLightHalfVec), 16.f), 0.f, 1.f);
	return lightColor * (pointLightLambert * albedo + pointLightBlinn * gloss);
}

// This function expects that we have :
// - a uniforms named bucketOffsetAndSize of ivec4[]
// - a uniforms named PointLightsPosition of vec4[]
// - a uniforms named colorAndEnergy of vec4[]
// fragNormal and view vector are expected to be in the spaceMatrix space
// spaceMatrix is used to move from world space to necessary space
vec4 iterateOverAllPointLights(
	vec2 clipSpaceCoord,
	vec3 WorldFragPos,
	vec3 fragNormal,
	vec3 viewVector,
	vec4 albedo,
	float gloss,
	mat3 spaceMatrix
) {
	vec4 light = vec4(0);
	ivec2 bucket = ivec2(bucketDimensionUsed * clipSpaceCoord);
	int bucketId = min(bucket.y + bucket.x * bucketDimensionUsed, bucketDimensionUsed * bucketDimensionUsed - 1);

	for (int i = 0; i < bucketOffsetAndSize[bucketId].y; i++)
	{
		int entryInLightList = bucketOffsetAndSize[bucketId].x + i;
		int lightIndex = PointLightsIndex[entryInLightList / 4][entryInLightList % 4];
		vec4 position = PointLightsPosition[lightIndex];
		vec4 colorAndEnergy = PointLightsColorAndEnergy[lightIndex];
		vec3 tmp = position.xyz * vec3(1., 1., -1.);
		// skip the (expensive) shadow ray entirely for lights whose energy at this fragment
		// is zero or negligible - the attenuation is hard-zero past the light's range
		float energy = pointLightEnergyAtPosition(tmp - WorldFragPos, colorAndEnergy.w);
		if (energy <= 0.004)
		{
			continue;
		}
		float occlusion = wzPointLightOcclusion(WorldFragPos, tmp);
		if (occlusion > 0.0)
		{
			light += occlusion * processPointLight(WorldFragPos, fragNormal, viewVector, albedo, gloss, tmp, colorAndEnergy.w, colorAndEnergy.xyz, spaceMatrix);
		}
	}
	return light;
}
