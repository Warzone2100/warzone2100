// The light arrays, the bucket grid and the viewport dimensions all arrive in the
// including stage's uniform block, which must be declared before this file.

// See https://lisyarus.github.io/blog/graphics/2022/07/30/point-light-attenuation.html for explanation
// we want something that looks somewhat physically correct, but must absolutely be 0 past range
float pointLightEnergyAtPosition(vec3 pointLightVector, float range)
{
	float normalizedDistance = length(pointLightVector) / range; //to-do: make range based on effect size
	float sqNormDist = normalizedDistance * normalizedDistance;
	float numerator = max(1.f - sqNormDist, 0.f);
	return numerator * numerator / ( 1.f + 2.f * sqNormDist);
}

// How much of the cone a fragment sits in.
// A cosOuter of -1 means the light has no cone (which is what every light without a direction carries), so this returns 1 and nothing changes.
// The inner edge is derived from the outer rather than stored, which keeps the transport to one vec4 per light and costs a constant.
float pointLightConeFactor(vec3 pointLightVector, vec4 directionAndCos)
{
	if (directionAndCos.w <= -1.f)
	{
		return 1.f;
	}
	// A wide falloff, because a hard rim on the ground makes a cone read as a hard beam rather than as something glowing.
	// (At 0.5 a 60 degree cone fades from about 40 degrees.)
	const float coneSoftness = 0.5f;
	float cosOuter = directionAndCos.w;
	float cosInner = mix(cosOuter, 1.f, coneSoftness);
	float alignment = dot(normalize(-pointLightVector), directionAndCos.xyz);
	return smoothstep(cosOuter, cosInner, alignment);
}

vec4 processPointLight(vec3 WorldFragPos, vec3 fragNormal, vec3 viewVector, vec4 albedo, float gloss, vec3 pointLightWorldPosition, float pointLightEnergy, vec3 pointLightColor, float pointLightIntensity, vec4 pointLightDirectionAndCos, mat3 spaceMatrix)
{
	vec3 pointLightVector = pointLightWorldPosition - WorldFragPos;
	vec3 pointLightDir = spaceMatrix * normalize(pointLightVector);

	float energy = pointLightEnergyAtPosition(pointLightVector, pointLightEnergy);
	energy *= pointLightIntensity * pointLightConeFactor(pointLightVector, pointLightDirectionAndCos);
	vec4 lightColor = vec4(pointLightColor * energy, 1.f); //to-do: pick average color from effect's texture
	//lightColor.rgb = mix(lightColor.rgb, (lightColor.rgb + 1.5)*energy*2, energy);

	float pointLightLambert = max(dot(fragNormal, pointLightDir), 0.f);
	vec3 pointLightHalfVec = normalize(pointLightDir + viewVector);
	float pointLightBlinn = clamp(pow(max(dot(fragNormal, pointLightHalfVec), 0.f), 16.f), 0.f, 1.f);
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
	return texelFetch(lightDataBuffer, lightIndex * 3);
}

vec4 wzLightColorAndEnergy(int lightIndex)
{
	return texelFetch(lightDataBuffer, lightIndex * 3 + 1);
}

vec4 wzLightDirectionAndCos(int lightIndex)
{
	return texelFetch(lightDataBuffer, lightIndex * 3 + 2);
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
	return lights[lightIndex * 3];
}

vec4 wzLightColorAndEnergy(int lightIndex)
{
	return lights[lightIndex * 3 + 1];
}

vec4 wzLightDirectionAndCos(int lightIndex)
{
	return lights[lightIndex * 3 + 2];
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

vec4 wzLightDirectionAndCos(int lightIndex)
{
	return PointLightsDirectionAndCos[lightIndex];
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
	vec4 light = vec4(0.f);
	ivec2 bucket = ivec2(float(bucketDimensionUsed) * clipSpaceCoord);
	int bucketId = min(bucket.y + bucket.x * bucketDimensionUsed, bucketDimensionUsed * bucketDimensionUsed - 1);

	for (int i = 0; i < bucketOffsetAndSize[bucketId].y; i++)
	{
		int entryInLightList = bucketOffsetAndSize[bucketId].x + i;
		int lightIndex = wzLightIndex(entryInLightList);
		vec4 position = wzLightPosition(lightIndex);
		vec4 colorAndEnergy = wzLightColorAndEnergy(lightIndex);
		vec4 directionAndCos = wzLightDirectionAndCos(lightIndex);
		// The cone direction arrives in the same space as the position, so it gets the same flip
		directionAndCos.xyz *= vec3(1.f, 1.f, -1.f);
		vec3 tmp = position.xyz * vec3(1.f, 1.f, -1.f);
		light += processPointLight(WorldFragPos, fragNormal, viewVector, albedo, gloss, tmp, colorAndEnergy.w, colorAndEnergy.xyz, position.w, directionAndCos, spaceMatrix);
	}
	return light;
}
