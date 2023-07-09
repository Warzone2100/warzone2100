#version 450
//#pragma debug(on)

#include "tcmask_instanced.glsl"

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;
layout (constant_id = 1) const uint WZ_EXTRA_SHADOW_TAPS = 4;
layout (constant_id = 2) const uint WZ_SHADOW_CASCADES_COUNT = 3;

layout(set = 2, binding = 0) uniform sampler2D Texture; // diffuse
layout(set = 2, binding = 1) uniform sampler2D TextureTcmask; // tcmask
layout(set = 2, binding = 2) uniform sampler2D TextureNormal; // normal map
layout(set = 2, binding = 3) uniform sampler2D TextureSpecular; // specular map
layout(set = 2, binding = 4) uniform sampler2DArrayShadow shadowMap; // shadow map

layout(location = 0) in vec4 texCoord_vertexDistance; // vec(2) texCoord, float vertexDistance, (unused float)
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 lightDir;
layout(location = 3) in vec3 halfVec;
layout(location = 4) in mat4 NormalMatrix;
layout(location = 8) in vec4 colour;
layout(location = 9) in vec4 teamcolour;
layout(location = 10) in vec4 packed_ecmState_alphaTest;
// For Shadows
layout(location = 11) in vec3 fragPos;
//layout(location = 13) in vec3 fragNormal;

layout(location = 0) out vec4 FragColor;

float getShadowVisibility()
{
if (WZ_EXTRA_SHADOW_TAPS == 0)
{
	return 1.0;
}
else
{
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

	float visibility = texture( shadowMap, vec4(pos.xy, cascadeIndex, (pos.z+bias)) );

	// PCF
	const float edgeVal = 0.5+float((WZ_EXTRA_SHADOW_TAPS-1)/2);
	const float startVal = -edgeVal;
	const float endVal = edgeVal + 0.5;
	const float texelIncrement = 1.0/float(4096);
	const float visibilityIncrement = 0.1; //0.5 / WZ_EXTRA_SHADOW_TAPS;
	for (float y=startVal; y<endVal; y+=1.0)
	{
		for (float x=startVal; x<endVal; x+=1.0)
		{
			visibility -= visibilityIncrement*(1.0-texture( shadowMap, vec4(pos.xy + vec2(x*texelIncrement, y*texelIncrement), cascadeIndex, (pos.z+bias)) ));
		}
	}

	visibility = clamp(visibility, 0.3, 1.0);

	return visibility;
}
}

void main()
{
	// unpack inputs
	vec2 texCoord = vec2(texCoord_vertexDistance.x, texCoord_vertexDistance.y);
	float vertexDistance = texCoord_vertexDistance.z;
	bool ecmEffect = (packed_ecmState_alphaTest.x > 0.f);
	bool alphaTest = (packed_ecmState_alphaTest.y > 0.f);

	vec4 diffuseMap = texture(Texture, texCoord, WZ_MIP_LOAD_BIAS);

	if (alphaTest && (diffuseMap.a <= 0.5))
	{
		discard;
	}

	// Normal map implementations
	vec3 N = normal;
	if (normalmap != 0)
	{
		vec3 normalFromMap = texture(TextureNormal, texCoord, WZ_MIP_LOAD_BIAS).xyz;

		// Complete replace normal with new value
		N = normalFromMap.xzy * 2.0 - 1.0;
		N.y = -N.y; // FIXME - to match WZ's light

		// For object-space normal map
		if (hasTangents == 0)
		{
			N = (NormalMatrix * vec4(N, 0.0)).xyz;
		}
	}
	N = normalize(N);

	// Сalculate and combine final lightning
	vec4 light = sceneColor;
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); //diffuse light
	float visibility = getShadowVisibility();

	if (lambertTerm > 0.0)
	{
		float vanillaFactor = 0.0; // Classic models shouldn't use diffuse light

		if (specularmap != 0)
		{
			float specularMapValue = texture(TextureSpecular, texCoord, WZ_MIP_LOAD_BIAS).r;
			vec4 specularFromMap = vec4(specularMapValue, specularMapValue, specularMapValue, 1.0);

			// Gaussian specular term computation
			vec3 H = normalize(halfVec);
			float exponent = acos(dot(H, N)) / 0.33; //0.33 is shininess
			float gaussianTerm = exp(-(exponent * exponent));

			light += specular * gaussianTerm * lambertTerm * specularFromMap;

			vanillaFactor = 1.0; // Neutralize factor for spec map
		}

		light += diffuse * lambertTerm * diffuseMap * vanillaFactor;
	}
	// ambient light maxed for classic models to keep results similar to original
	light += ambient * diffuseMap * (1.0 + (1.0 - float(specularmap)));

	vec4 fragColour;
	if (tcmask != 0)
	{
		// Get mask for team colors from texture
		float maskAlpha = texture(TextureTcmask, texCoord, WZ_MIP_LOAD_BIAS).r;

		// Apply color using grain merge with tcmask
		fragColour = visibility * (light + (teamcolour - 0.5) * maskAlpha) * colour;
	}
	else
	{
		fragColour = visibility * light * colour;
	}

	if (ecmEffect)
	{
		fragColour.a = 0.66 + 0.66 * graphicsCycle;
	}
	
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColour = mix(fragColour, vec4(fogColor.xyz, fragColour.w), fogFactor);
	}

	FragColor = fragColour;
}
