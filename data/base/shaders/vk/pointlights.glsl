// See https://lisyarus.github.io/blog/graphics/2022/07/30/point-light-attenuation.html for explanation
// we want something that looks somewhat physically correct, but must absolutely be 0 past range
float pointLightEnergyAtPosition(vec3 position, vec3 pointLightWorldPosition, float range)
{
	vec3 pointLightVector = position - pointLightWorldPosition;
	float normalizedDistance = length(pointLightVector) / range;

	float sqNormDist =  normalizedDistance * normalizedDistance;
	float numerator = max(1 - sqNormDist, 0);
	return numerator * numerator / ( 1 + 2 * sqNormDist);
}

vec4 processPointLight(vec3 WorldFragPos, vec3 fragNormal, vec3 viewVector, vec4 albedo, float gloss, vec3 pointLightWorldPosition, float pointLightEnergy, vec3 pointLightColor, mat3 normalWorldSpaceToLocalSpace)
{
	vec3 pointLightVector = WorldFragPos - pointLightWorldPosition;
	vec3 pointLightDir = -normalize(pointLightVector * normalWorldSpaceToLocalSpace);

	float energy = pointLightEnergyAtPosition(WorldFragPos, pointLightWorldPosition, pointLightEnergy);
	vec4 lightColor = vec4(pointLightColor * energy, 1);

	float pointLightLambert = max(dot(fragNormal, pointLightDir), 0.0);

	vec3 pointLightHalfVec = normalize(viewVector + pointLightDir);

	float pointLightBlinn = pow(clamp(dot(fragNormal, pointLightHalfVec), 0.f, 1.f), 16.f);
	return lightColor * pointLightLambert * (albedo +  pointLightBlinn * (gloss * gloss));
}

// This function expects that we have :
// - a uniforms named bucketOffsetAndSize of ivec4[]
// - a uniforms named PointLightsPosition of vec4[]
// - a uniforms named colorAndEnergy of vec4[]
// fragNormal and view vector are expected to be in the same local space
// normalWorldSpaceToLocalSpace is used to move from world space to local space
vec4 iterateOverAllPointLights(
	vec2 clipSpaceCoord,
	vec3 WorldFragPos,
	vec3 fragNormal,
	vec3 viewVector,
	vec4 albedo,
	float gloss,
	mat3 normalWorldSpaceToLocalSpace
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
		light += processPointLight(WorldFragPos, fragNormal, viewVector, albedo, gloss, tmp, colorAndEnergy.w, colorAndEnergy.xyz, normalWorldSpaceToLocalSpace);
	}
	return light;
}
