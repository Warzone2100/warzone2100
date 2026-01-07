// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
#define WZ_SHADOW_MODE 1
#define WZ_SHADOW_FILTER_SIZE 3
#define WZ_SHADOW_CASCADES_COUNT 3
#define WZ_POINT_LIGHT_ENABLED 0
//

#define WZ_MAX_SHADOW_CASCADES 3

uniform sampler2D Texture; // diffuse map
uniform sampler2D TextureTcmask; // tcmask
uniform sampler2D TextureNormal; // normal map
uniform sampler2D TextureSpecular; // specular map
uniform sampler2DArrayShadow shadowMap; // shadow map
uniform sampler2D lightmap_tex;

uniform mat4 ViewMatrix;

uniform int tcmask; // whether a tcmask texture exists for the model
uniform int normalmap; // whether a normal map exists for the model
uniform int specularmap; // whether a specular map exists for the model
uniform int hasTangents; // whether tangents were calculated for model
uniform int shieldEffect;
uniform float graphicsCycle; // a periodically cycling value for special effects

uniform vec4 cameraPos; // in modelSpace

uniform vec4 sceneColor; //emissive light
uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 specular;

uniform mat4 ShadowMapMVPMatrix[WZ_MAX_SHADOW_CASCADES];
uniform vec4 ShadowMapCascadeSplits;
uniform int ShadowMapSize;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#define FRAGMENT_INPUT in
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#define FRAGMENT_INPUT varying
#endif

FRAGMENT_INPUT vec3 normal;
FRAGMENT_INPUT vec3 lightDir;
FRAGMENT_INPUT vec3 halfVec;
FRAGMENT_INPUT mat3 NormalMatrix;
FRAGMENT_INPUT mat3 TangentSpaceMatrix;
FRAGMENT_INPUT vec4 colour;
FRAGMENT_INPUT vec4 teamcolour;
FRAGMENT_INPUT vec4 packed_ecmState_alphaTest_texCoord;
FRAGMENT_INPUT vec3 uvLightmap; // uvLightmap in .xy, heightAboveTerrain in .z

// For Shadows
FRAGMENT_INPUT vec3 posModelSpace;
FRAGMENT_INPUT vec3 posViewSpace;

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

#if WZ_POINT_LIGHT_ENABLED == 1
#include "pointlights.frag"
#endif
#include "terrain_combined_frag.glsl"

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
	float lambertTerm = max(dot(N, L), 0.0); //diffuse light
	float visibility = getShadowVisibility(N, L, 0.f);
	vec4 lightmap_vec4 = texture(lightmap_tex, uvLightmap.xy, 0.f);
	float distanceAboveTerrain = uvLightmap.z;
	float lightmapFactor = 1.0f - (clamp(distanceAboveTerrain, 0.f, 300.f) / 300.f);

	float specularMapValue = 0.f;

	if (lambertTerm > 0.0)
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

			light += specular * gaussianTerm * lambertTerm * specularFromMap;

			vanillaFactor = 1.0; // Neutralize factor for spec map
		}

		light += diffuse * lambertTerm * diffuseMap * vanillaFactor;
	}
	// ambient light maxed for classic models to keep results similar to original
	light += vec4(blendAddEffectLighting(ambient.rgb, ((lightmap_vec4.rgb * lightmapFactor) / 3.f)), ambient.a) * diffuseMap * (1.0 + (1.0 - float(specularmap)));

#if WZ_POINT_LIGHT_ENABLED == 1
	vec2 clipSpaceCoord = gl_FragCoord.xy / vec2(float(viewportWidth), float(viewportHeight));
	light += iterateOverAllPointLights(clipSpaceCoord, posModelSpace, N, normalize(halfVec - lightDir), diffuseMap, specularMapValue, mat3(1));
#endif

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

	#ifdef NEWGL
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
