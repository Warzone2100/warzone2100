// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
layout(std140) uniform globaluniforms {
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ShadowMapMVPMatrix;
	vec4 cameraPos;
	vec4 lightPosition;
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 fogColor;
	vec4 fogRange;
	float graphicsCycle;
	float WZ_MIP_LOAD_BIAS;
	float pad0;
	float pad1;
};

layout(std140) uniform meshuniforms {
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
	int fogOutput;
	int encodeOutput;
	int linearLighting;
};

layout(std140) uniform instanceuniforms {
	mat4 ModelMatrix;
	mat4 NormalMatrix;
	vec4 colour;
	vec4 teamcolour;
	float stretch;
	float animFrameNumber;
	int ecmEffect;
	int alphaTest;
};
//

uniform sampler2D Texture; // diffuse map
uniform sampler2D TextureTcmask; // tcmask
uniform sampler2D TextureNormal; // normal map
uniform sampler2D TextureSpecular; // specular map



#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#endif

#ifdef NEWGL
in vec3 posViewSpace;
in vec3 normal;
in vec3 lightDir;
in vec3 halfVec;
in vec2 texCoord;
in mat3 TangentSpaceMatrix;
#else
varying vec3 posViewSpace;
varying vec3 normal;
varying vec3 lightDir;
varying vec3 halfVec;
varying vec2 texCoord;
varying mat3 TangentSpaceMatrix;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

#include "tangentspace.glsl"
#include "distance_fog.glsl"
#include "srgb.glsl"

void main()
{
	vec4 diffuseMap = texture(Texture, texCoord, WZ_MIP_LOAD_BIAS);

	if ((alphaTest != 0) && (diffuseMap.a <= 0.5))
	{
		discard;
	}

	// Normal map implementations
	vec3 N = normal;
	if (normalmap != 0)
	{
		vec3 normalFromMap = texture(TextureNormal, texCoord, WZ_MIP_LOAD_BIAS).xyz;

		N = wzDecodeNormalMap(normalFromMap, hasTangents, TangentSpaceMatrix, mat3(NormalMatrix));
	}
	N = normalize(N);

	// Сalculate and combine final lightning
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); //diffuse light
	// ambient light maxed for classic models to keep results similar to original
	float ambientBoost = 1.0 + (1.0 - float(specularmap));

	float specularMapValue = 0.f;
	float gaussianTerm = 0.f;
	float vanillaFactor = 0.0; // Classic models shouldn't use diffuse light
	if (lambertTerm > 0.0 && specularmap != 0)
	{
		specularMapValue = texture(TextureSpecular, texCoord, WZ_MIP_LOAD_BIAS).r;

		// Gaussian specular term computation
		vec3 H = normalize(halfVec);
		float exponent = acos(dot(H, N)) / 0.33; //0.33 is shininess
		gaussianTerm = exp(-(exponent * exponent));

		vanillaFactor = 1.0; // Neutralize factor for spec map
	}

	// Calculate and combine final lighting
	vec4 light;
	if (linearLighting != 0)
	{
		// The model lighting was tuned for display values, so it is evaluated in display space
		vec3 albedoD = wzLinearToSrgb(diffuseMap.rgb);
		vec3 lightD = wzLinearToSrgb(sceneColor.rgb);
		lightD += wzLinearToSrgb(specular.rgb) * gaussianTerm * lambertTerm * specularMapValue;
		lightD += wzLinearToSrgb(diffuse.rgb) * lambertTerm * albedoD * vanillaFactor;
		lightD += wzLinearToSrgb(ambient.rgb) * albedoD * ambientBoost;
		light = vec4(wzSrgbToLinear(lightD), 1.0);
	}
	else
	{
		light = sceneColor;
		light += specular * gaussianTerm * lambertTerm * vec4(vec3(specularMapValue), 1.0);
		light += diffuse * lambertTerm * diffuseMap * vanillaFactor;
		vec4 boostedAmbient = ambient * diffuseMap;
		boostedAmbient.rgb *= ambientBoost;
		light += boostedAmbient;
	}

	vec4 fragColour;
	if (tcmask != 0)
	{
		// Get mask for team colors from texture
		float maskAlpha = texture(TextureTcmask, texCoord, WZ_MIP_LOAD_BIAS).r;

		// Apply color using grain merge with tcmask
		if (linearLighting != 0)
		{
			// Grain merge (neutral at 0.5) and the color multiply also run in display space, as tuned
			vec4 blended = vec4(wzLinearToSrgb(light.rgb), light.a) + (teamcolour - 0.5) * maskAlpha;
			fragColour = vec4(wzSrgbToLinear(max(blended.rgb, vec3(0.0)) * wzLinearToSrgb(colour.rgb)), blended.a * colour.a);
		}
		else
		{
			vec4 blended = light + (teamcolour - 0.5) * maskAlpha;
			fragColour = blended * colour;
		}
	}
	else
	{
		fragColour = (linearLighting != 0)
			? vec4(wzSrgbToLinear(wzLinearToSrgb(light.rgb) * wzLinearToSrgb(colour.rgb)), light.a * colour.a)
			: light * colour;
	}

	if (ecmEffect > 0)
	{
		fragColour.a = 0.66 + 0.66 * graphicsCycle;
	}
	if (fogRange.z > 0.5 && fogOutput != WZ_FOG_OUTPUT_DISABLED)
	{
		float fogAmount = wzDistanceFogAmount(length(posViewSpace), fogRange.x, fogRange.y);
		fragColour.rgb = wzApplyForwardFog(fragColour.rgb, fragColour.a, fogAmount, fogColor.rgb, fogOutput, int(fogRange.w));
	}

	if (encodeOutput != 0)
	{
		// Drawn outside the scene, so nothing downstream will encode this for display
		fragColour.rgb = wzLinearToSrgb(fragColour.rgb);
	}

	#ifdef NEWGL
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
