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

// Accessors for the light arrays. The loop below reads light data only through these.
#if WZ_LIGHT_TRANSPORT == 1
// A buffer texture carries one format, so the two light vec4s share a float buffer at a
// stride of two texels and the index list gets an integer buffer of its own.
uniform samplerBuffer lightDataBuffer;
uniform isamplerBuffer lightIndexBuffer;

vec4 wzLightPosition(int lightIndex)
{
	return texelFetch(lightDataBuffer, lightIndex * 2);
}

vec4 wzLightColorAndEnergy(int lightIndex)
{
	return texelFetch(lightDataBuffer, lightIndex * 2 + 1);
}

int wzLightIndex(int entryInLightList)
{
	return texelFetch(lightIndexBuffer, entryInLightList).r;
}
#elif WZ_LIGHT_TRANSPORT == 2
// std430 packs both of these exactly as the buffer texture path lays them out
layout(std430) readonly buffer lightData { vec4 lights[]; };
layout(std430) readonly buffer lightIndexData { int lightIndices[]; };

vec4 wzLightPosition(int lightIndex)
{
	return lights[lightIndex * 2];
}

vec4 wzLightColorAndEnergy(int lightIndex)
{
	return lights[lightIndex * 2 + 1];
}

int wzLightIndex(int entryInLightList)
{
	return lightIndices[entryInLightList];
}
#else
vec4 wzLightPosition(int lightIndex)
{
	return PointLightsPosition[lightIndex];
}

vec4 wzLightColorAndEnergy(int lightIndex)
{
	return PointLightsColorAndEnergy[lightIndex];
}

int wzLightIndex(int entryInLightList)
{
	return PointLightsIndex[entryInLightList / 4][entryInLightList % 4];
}
#endif

// This function expects that we have :
// - a uniform named bucketOffsetAndSize of ivec4[]
// - a uniform named bucketDimensionUsed of int
// - the light accessors declared above
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
		int lightIndex = wzLightIndex(entryInLightList);
		vec4 position = wzLightPosition(lightIndex);
		vec4 colorAndEnergy = wzLightColorAndEnergy(lightIndex);
		vec3 tmp = position.xyz * vec3(1., 1., -1.);
		light += processPointLight(WorldFragPos, fragNormal, viewVector, albedo, gloss, tmp, colorAndEnergy.w, colorAndEnergy.xyz, spaceMatrix);
	}
	return light;
}
