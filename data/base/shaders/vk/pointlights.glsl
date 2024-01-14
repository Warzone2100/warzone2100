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

// InpoutLightDirection, N and H are expected to be normalized, and in the same space (eg global/tangent/etc)
// Based on Blinn Phong model
vec3 BRDF(vec3 inputLightDirection, vec3 incomingLightEnergy, vec3 N, vec3 H, MaterialInfo materialInfo)
{
	// diffuse lighting
	float lambertTerm = max(dot(N, inputLightDirection), 0.0);
	vec3 diffuseComponent = materialInfo.albedo.xyz * incomingLightEnergy * lambertTerm;

	// Gaussian specular term computation
	float blinnTerm = clamp(dot(N, H), 0.f, 1.f);
	blinnTerm = pow(blinnTerm, 16.f);
	vec3 blinnComponent = incomingLightEnergy * (materialInfo.gloss * materialInfo.gloss) * blinnTerm;

	return diffuseComponent + blinnComponent;
}


vec4 processPointLight(vec3 WorldFragPos, vec3 fragNormal, vec3 viewVector, MaterialInfo material, vec3 pointLightWorldPosition, float pointLightEnergy, vec3 pointLightColor, mat3 normalWorldSpaceToLocalSpace)
{
	vec3 pointLightVector = WorldFragPos - pointLightWorldPosition;
	vec3 pointLightDir = -normalize(pointLightVector * normalWorldSpaceToLocalSpace);

	float energy = pointLightEnergyAtPosition(WorldFragPos, pointLightWorldPosition, pointLightEnergy);	
	vec3 pointLightHalfVec = normalize(viewVector + pointLightDir);

	return vec4(BRDF(pointLightDir, energy * pointLightColor, fragNormal, pointLightHalfVec, material), 1);
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
		vec3 tmp = position.xyz * vec3(1., 1., -1.);
		light += processPointLight(WorldFragPos, fragNormal, viewVector, material, tmp, colorAndEnergy.w, colorAndEnergy.xyz, normalWorldSpaceToLocalSpace);
	}
	return light;
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


#define STEPS 64
	for (int i = 0; i < STEPS; i++)
	{
		
		vec3 posOnViewLine = WorldFragPos + viewLine * i / STEPS;
		// fog is thicker near 0
		float thickness = exp(-posOnViewLine.y / 300);

		vec3 od = fogColor.xyz * thickness * length(viewLine / STEPS) / 1000;

		float sunLightEnergy = getShadowVisibility(posOnViewLine) ;
		vec3 scatteredLight = vec3(sunLightEnergy) * sunLightColor * od;

		for (int i = 0; i < bucketOffsetAndSize[bucketId].y; i++)
		{
			int entryInLightList = bucketOffsetAndSize[bucketId].x + i;
			int lightIndex = PointLightsIndex[entryInLightList / 4][entryInLightList % 4];
			vec4 position = PointLightsPosition[lightIndex];
			vec4 colorAndEnergy = PointLightsColorAndEnergy[lightIndex];
			vec3 tmp = position.xyz * vec3(1., 1., -1.);
			scatteredLight += colorAndEnergy.xyz * pointLightEnergyAtPosition(posOnViewLine, tmp, colorAndEnergy.w) * od;
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