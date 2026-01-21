#version 450
//#pragma debug(on)

#include "tcmask_instanced.glsl"

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;
layout (constant_id = 1) const uint WZ_SHADOW_MODE = 1;
layout (constant_id = 2) const uint WZ_SHADOW_FILTER_SIZE = 5;
layout (constant_id = 3) const uint WZ_SHADOW_CASCADES_COUNT = 3;
layout (constant_id = 4) const uint WZ_POINT_LIGHT_ENABLED = 0;

layout(set = 2, binding = 0) uniform sampler2D Texture; // diffuse
layout(set = 2, binding = 1) uniform sampler2D TextureTcmask; // tcmask
layout(set = 2, binding = 2) uniform sampler2D TextureNormal; // normal map
layout(set = 2, binding = 3) uniform sampler2D TextureSpecular; // specular map
layout(set = 2, binding = 4) uniform sampler2DArrayShadow shadowMap; // shadow map
layout(set = 2, binding = 5) uniform sampler2D lightmap_tex;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 lightDir;
layout(location = 2) in vec3 halfVec;
layout(location = 3) in mat3 NormalMatrix; // local to model space for directional vectors
layout(location = 6) in mat3 TangentSpaceMatrix;
layout(location = 9) in vec4 colour;
layout(location = 10) in vec4 teamcolour;
layout(location = 11) in vec4 packed_ecmState_alphaTest_texCoord;
layout(location = 12) in vec3 uvLightmap; // uvLightmap in .xy, heightAboveTerrain in .z
// For Shadows
layout(location = 13) in vec3 posModelSpace;
layout(location = 14) in vec3 posViewSpace;

layout(location = 0) out vec4 FragColor;

#include "pointlights.glsl"
#include "terrain_combined_frag.glsl"
#include "light.glsl"

float random(vec2 uv)
{
	return fract(sin(dot(uv.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

vec4 applyShieldFuzzEffect(vec4 color) {
	float cycle = 0.66 + 0.66 * graphicsCycle;
	vec3 col = vec3(random(vec2(posModelSpace.x * cycle, posModelSpace.y * cycle)));
	col.r *= 0.5;
	col.g *= 0.5;
	col.b *= 2.0;
	return vec4(col, color.a / 6.0);
}

void main()
{
	// unpack inputs
	vec2 texCoord = vec2(packed_ecmState_alphaTest_texCoord.z, packed_ecmState_alphaTest_texCoord.w);
	bool ecmEffect = (packed_ecmState_alphaTest_texCoord.x > 0.f);
	bool alphaTest = (packed_ecmState_alphaTest_texCoord.y > 0.f);

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

		// transform tangent-space normal map into world space
		N = normalFromMap.xzy * 2.0 - 1.0;
		N = TangentSpaceMatrix * N;

		if (hasTangents == 0)
		{
			// transform object-space normal map into world space
			N = normalFromMap.xzy * 2.0 - 1.0;
			N = NormalMatrix * vec3(-N.x, N.y, -N.z);
		}
	}
	N = normalize(N);

	// Сalculate and combine final lightning
	vec4 light = sceneColor;
	vec3 L = normalize(lightDir);
	float diffuseFactor = lambertTerm(N, L); //diffuse light
	float visibility = getShadowVisibility(posModelSpace, posViewSpace, diffuseFactor, 0.0f);
	vec4 lightmap_vec4 = texture(lightmap_tex, uvLightmap.xy, 0.f);
	float distanceAboveTerrain = uvLightmap.z;
	float lightmapFactor = 1.0f - (clamp(distanceAboveTerrain, 0.f, 300.f) / 300.f);

	float specularMapValue = 0.f;
	if (diffuseFactor > 0.0)
	{
		float vanillaFactor = 0.0; // Classic models shouldn't use diffuse light

		if (specularmap != 0)
		{
			specularMapValue = texture(TextureSpecular, texCoord, WZ_MIP_LOAD_BIAS).r;
			vec4 specularFromMap = vec4(specularMapValue, specularMapValue, specularMapValue, 1.0);

			// Gaussian specular term computation
			vec3 H = normalize(halfVec);
			float exponent = acos(dot(H, N)) / 0.33; //0.33 is shininess
			float gaussianTerm = exp(-(exponent * exponent));

			light += specular * gaussianTerm * diffuseFactor * specularFromMap;

			vanillaFactor = 1.0; // Neutralize factor for spec map
		}

		light += diffuse * diffuseFactor * diffuseMap * vanillaFactor;
	}
	// ambient light maxed for classic models to keep results similar to original
	light += vec4(blendAddEffectLighting(ambient.rgb, ((lightmap_vec4.rgb * lightmapFactor) / 3.f)), ambient.a) * diffuseMap * (1.0 + (1.0 - float(specularmap)));

	if (WZ_POINT_LIGHT_ENABLED == 1)
	{
		vec2 clipSpaceCoord = gl_FragCoord.xy / vec2(viewportWidth, viewportHeight);
		light += iterateOverAllPointLights(clipSpaceCoord, posModelSpace, N, normalize(halfVec - lightDir), diffuseMap, specularMapValue, mat3(1.f));
	}

	light.rgb *= visibility;
	light.a = 1.0f;

	vec4 fragColour;
	if (tcmask != 0)
	{
		// Get mask for team colors from texture
		float maskAlpha = texture(TextureTcmask, texCoord, WZ_MIP_LOAD_BIAS).r;

		// Apply color using grain merge with tcmask
		fragColour = (light + (teamcolour - 0.5) * maskAlpha) * colour;
	}
	else
	{
		fragColour = light * colour;
	}

	if (ecmEffect)
	{
		fragColour.a = 0.66 + 0.66 * graphicsCycle;
	}
	
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - length(posViewSpace)) / (fogEnd - fogStart);

		// Return fragment color
		fragColour = mix(fragColour, vec4(fogColor.xyz, fragColour.w), clamp(fogFactor, 0.0, 1.0));
	}

	if (shieldEffect != 0)
	{
		fragColour = applyShieldFuzzEffect(fragColour);
	}

	FragColor = fragColour;
}
