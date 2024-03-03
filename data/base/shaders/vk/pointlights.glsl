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

struct MaterialInfo
{
	vec4 albedo;
	float gloss;
};

vec4 processPointLight(vec3 WorldFragPos, vec3 fragNormal, vec3 viewVector, MaterialInfo material, vec3 pointLightWorldPosition, float pointLightEnergy, vec3 pointLightColor, mat3 normalWorldSpaceToLocalSpace)
{
	vec3 pointLightVector = WorldFragPos - pointLightWorldPosition;
	vec3 pointLightDir = -normalize(pointLightVector * normalWorldSpaceToLocalSpace);

	float energy = pointLightEnergyAtPosition(WorldFragPos, pointLightWorldPosition, pointLightEnergy);
	vec4 lightColor = vec4(pointLightColor * energy, 1);

	float pointLightLambert = max(dot(fragNormal, pointLightDir), 0.0);

	vec3 pointLightHalfVec = normalize(viewVector + pointLightDir);

	float pointLightBlinn = pow(clamp(dot(fragNormal, pointLightHalfVec), 0.f, 1.f), 16.f);
	return lightColor * pointLightLambert * (material.albedo +  pointLightBlinn * (material.gloss * material.gloss));
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
	MaterialInfo material,
	mat3 normalWorldSpaceToLocalSpace
) {
	vec4 light = vec4(0);
	ivec2 bucket = ivec2(WZ_BUCKET_DIMENSION * clipSpaceCoord);
	int bucketId = min(bucket.y + bucket.x * WZ_BUCKET_DIMENSION, WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION - 1);

	for (int i = 0; i < bucketOffsetAndSize[bucketId].y; i++)
	{
		int entryInLightList = bucketOffsetAndSize[bucketId].x + i;
		int lightIndex = PointLightsIndex[entryInLightList / 4][entryInLightList % 4];
		vec4 position = PointLightsPosition[lightIndex];
		vec4 colorAndEnergy = PointLightsColorAndEnergy[lightIndex];
		light += processPointLight(WorldFragPos, fragNormal, viewVector, material, position.xyz, colorAndEnergy.w, colorAndEnergy.xyz, normalWorldSpaceToLocalSpace);
	}
	return light;
}


float getShadowVisibilityWithoutPCF(vec3 fragPos)
{
	if (WZ_SHADOW_MODE == 0 || WZ_SHADOW_FILTER_SIZE == 0)
	{
		// no shadow-mapping
		return 1.0;
	}
	else
	{
		// Shadow Mapping

		vec4 fragPosViewSpace = ViewMatrix * vec4(fragPos, 1.0);
		float depthValue = abs(fragPosViewSpace.z);

		int cascadeIndex = 0;

		// unrolled loop, using vec4 swizzles
		if (WZ_SHADOW_CASCADES_COUNT > 1)
		{
			if (depthValue >= ShadowMapCascadeSplits.x)
			{
				cascadeIndex = 1;
			}
		}
		if (WZ_SHADOW_CASCADES_COUNT > 2)
		{
			if (depthValue >= ShadowMapCascadeSplits.y)
			{
				cascadeIndex = 2;
			}
		}
		if (WZ_SHADOW_CASCADES_COUNT > 3)
		{
			if (depthValue >= ShadowMapCascadeSplits.z)
			{
				cascadeIndex = 3;
			}
		}

		vec4 shadowPos = ShadowMapMVPMatrix[cascadeIndex] * vec4(fragPos, 1.0);
		vec3 pos = shadowPos.xyz / shadowPos.w;

		if (pos.z > 1.0f)
		{
			return 1.0;
		}

		float bias = 0.0002f;

		return  texture( shadowMap, vec4(pos.xy, cascadeIndex, (pos.z+bias)) );
	}
}


// based on equations found here : https://www.shadertoy.com/view/lstfR7
vec4 volumetricLights(
	vec2 clipSpaceCoord,
	vec3 cameraPosition,
	vec3 WorldFragPos,
	vec3 sunLightColor
) {
	vec3 result = vec3(0);
	ivec2 bucket = ivec2(WZ_BUCKET_DIMENSION * clipSpaceCoord);
	int bucketId = min(bucket.y + bucket.x * WZ_BUCKET_DIMENSION, WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION - 1);


	vec3 viewLine = cameraPosition.xyz - WorldFragPos;
	vec3 currentTransmittence = vec3(1);
	vec3 transMittance = vec3(1);


#define STEPS (WZ_VOLUMETRIC_LIGHTING_ENABLED * 16)
	for (int i = 0; i < STEPS; i++)
	{
		
		vec3 posOnViewLine = WorldFragPos + viewLine * i / STEPS;
		// fog is thicker near 0
		float thickness = exp(-posOnViewLine.y / 300);

		vec3 od = fogColor.xyz * thickness * length(viewLine / STEPS) / 1000;

		float sunLightEnergy = getShadowVisibilityWithoutPCF(posOnViewLine) ;
		vec3 scatteredLight = vec3(sunLightEnergy) * sunLightColor * od;

		for (int i = 0; i < bucketOffsetAndSize[bucketId].y; i++)
		{
			int entryInLightList = bucketOffsetAndSize[bucketId].x + i;
			int lightIndex = PointLightsIndex[entryInLightList / 4][entryInLightList % 4];
			vec4 position = PointLightsPosition[lightIndex];
			vec4 colorAndEnergy = PointLightsColorAndEnergy[lightIndex];
			scatteredLight += colorAndEnergy.xyz * pointLightEnergyAtPosition(posOnViewLine, position.xyz, colorAndEnergy.w) * od;
		}

		result += scatteredLight * currentTransmittence;

		currentTransmittence *= exp2(od);
		transMittance *= exp2(-od);
	}
	return vec4(result * transMittance, transMittance);
}

vec3 toneMap(vec3 x)
{
	return x;
}