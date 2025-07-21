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

FRAGMENT_INPUT vec4 texCoord_vertexDistance; // vec(2) texCoord, float vertexDistance, (unused float)
FRAGMENT_INPUT vec3 normal;
FRAGMENT_INPUT vec3 lightDir;
FRAGMENT_INPUT vec3 halfVec;
FRAGMENT_INPUT mat4 NormalMatrix;
FRAGMENT_INPUT vec4 colour;
FRAGMENT_INPUT vec4 teamcolour;
FRAGMENT_INPUT vec4 packed_ecmState_alphaTest;
FRAGMENT_INPUT vec3 uvLightmap; // uvLightmap in .xy, heightAboveTerrain in .z

// For Shadows
FRAGMENT_INPUT vec3 fragPos;
FRAGMENT_INPUT vec3 fragNormal;

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

#if WZ_POINT_LIGHT_ENABLED == 1
#include "pointlights.frag"
#endif

float random(vec2 uv)
{
	return fract(sin(dot(uv.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float getShadowMapDepthComp(vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, int cascadeIndex, float z)
{
	vec2 uv = base_uv + vec2(u, v) * shadowMapSizeInv;
	return texture( shadowMap, vec4(uv, cascadeIndex, z) );
}

float getShadowVisibility()
{
#if WZ_SHADOW_MODE == 0 || WZ_SHADOW_FILTER_SIZE == 0
	// no shadow-mapping
	return 1.0;
#else
	// Shadow Mapping

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
#if WZ_SHADOW_CASCADES_COUNT > 1
	if (depthValue >= ShadowMapCascadeSplits.x)
	{
		cascadeIndex = 1;
	}
#endif
#if WZ_SHADOW_CASCADES_COUNT > 2
	if (depthValue >= ShadowMapCascadeSplits.y)
	{
		cascadeIndex = 2;
	}
#endif
#if WZ_SHADOW_CASCADES_COUNT > 3
	if (depthValue >= ShadowMapCascadeSplits.z)
	{
		cascadeIndex = 3;
	}
#endif

	vec4 shadowPos = ShadowMapMVPMatrix[cascadeIndex] * vec4(fragPos, 1.0);
	vec3 pos = shadowPos.xyz / shadowPos.w;

	if (pos.z > 1.0f)
	{
		return 1.0;
	}

	float bias = 0.0002f;

	float visibility = 0.f;

#if WZ_SHADOW_MODE == 1

	// Optimized PCF algorithm
	// See: https://therealmjp.github.io/posts/shadow-maps/
	// And: http://www.ludicon.com/castano/blog/articles/shadow-mapping-summary-part-1/

	vec2 shadowMapSize = vec2(float(ShadowMapSize), float(ShadowMapSize));

	float lightDepth = pos.z;

	lightDepth += bias;

	vec2 uv = pos.xy * shadowMapSize; // 1 unit - 1 texel

	vec2 shadowMapSizeInv = 1.0 / shadowMapSize;

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	float sum = 0.0;

	#if WZ_SHADOW_FILTER_SIZE <= 3
	// 3x3

	float uw0 = (3.0 - 2.0 * s);
	float uw1 = (1.0 + 2.0 * s);

	float u0 = (2.0 - s) / uw0 - 1.0;
	float u1 = s / uw1 + 1.0;

	float vw0 = (3.0 - 2.0 * t);
	float vw1 = (1.0 + 2.0 * t);

	float v0 = (2.0 - t) / vw0 - 1.0;
	float v1 = t / vw1 + 1.0;

	sum += uw0 * vw0 * getShadowMapDepthComp(base_uv, u0, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw0 * getShadowMapDepthComp(base_uv, u1, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw0 * vw1 * getShadowMapDepthComp(base_uv, u0, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw1 * getShadowMapDepthComp(base_uv, u1, v1, shadowMapSizeInv, cascadeIndex, lightDepth);

	visibility = sum * 1.0 / 16.0;

	#elif WZ_SHADOW_FILTER_SIZE > 3 && WZ_SHADOW_FILTER_SIZE <= 5
	// 5x5

	float uw0 = (4.0 - 3.0 * s);
	float uw1 = 7.0;
	float uw2 = (1.0 + 3.0 * s);

	float u0 = (3.0 - 2.0 * s) / uw0 - 2.0;
	float u1 = (3.0 + s) / uw1;
	float u2 = s / uw2 + 2.0;

	float vw0 = (4.0 - 3.0 * t);
	float vw1 = 7.0;
	float vw2 = (1.0 + 3.0 * t);

	float v0 = (3.0 - 2.0 * t) / vw0 - 2.0;
	float v1 = (3.0 + t) / vw1;
	float v2 = t / vw2 + 2.0;

	sum += uw0 * vw0 * getShadowMapDepthComp(base_uv, u0, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw0 * getShadowMapDepthComp(base_uv, u1, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw0 * getShadowMapDepthComp(base_uv, u2, v0, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw1 * getShadowMapDepthComp(base_uv, u0, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw1 * getShadowMapDepthComp(base_uv, u1, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw1 * getShadowMapDepthComp(base_uv, u2, v1, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw2 * getShadowMapDepthComp(base_uv, u0, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw2 * getShadowMapDepthComp(base_uv, u1, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw2 * getShadowMapDepthComp(base_uv, u2, v2, shadowMapSizeInv, cascadeIndex, lightDepth);

	visibility = sum * 1.0 / 144.0;

	#else
	// WZ_SHADOW_FILTER_SIZE > 5
	// 7x7

	float uw0 = (5.0 * s - 6.0);
	float uw1 = (11.0 * s - 28.0);
	float uw2 = -(11.0 * s + 17.0);
	float uw3 = -(5.0 * s + 1.0);

	float u0 = (4.0 * s - 5.0) / uw0 - 3.0;
	float u1 = (4.0 * s - 16.0) / uw1 - 1.0;
	float u2 = -(7.0 * s + 5.0) / uw2 + 1.0;
	float u3 = -s / uw3 + 3.0;

	float vw0 = (5.0 * t - 6.0);
	float vw1 = (11.0 * t - 28.0);
	float vw2 = -(11.0 * t + 17.0);
	float vw3 = -(5.0 * t + 1.0);

	float v0 = (4.0 * t - 5.0) / vw0 - 3.0;
	float v1 = (4.0 * t - 16.0) / vw1 - 1.0;
	float v2 = -(7.0 * t + 5.0) / vw2 + 1.0;
	float v3 = -t / vw3 + 3.0;

	sum += uw0 * vw0 * getShadowMapDepthComp(base_uv, u0, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw0 * getShadowMapDepthComp(base_uv, u1, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw0 * getShadowMapDepthComp(base_uv, u2, v0, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw3 * vw0 * getShadowMapDepthComp(base_uv, u3, v0, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw1 * getShadowMapDepthComp(base_uv, u0, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw1 * getShadowMapDepthComp(base_uv, u1, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw1 * getShadowMapDepthComp(base_uv, u2, v1, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw3 * vw1 * getShadowMapDepthComp(base_uv, u3, v1, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw2 * getShadowMapDepthComp(base_uv, u0, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw2 * getShadowMapDepthComp(base_uv, u1, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw2 * getShadowMapDepthComp(base_uv, u2, v2, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw3 * vw2 * getShadowMapDepthComp(base_uv, u3, v2, shadowMapSizeInv, cascadeIndex, lightDepth);

	sum += uw0 * vw3 * getShadowMapDepthComp(base_uv, u0, v3, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw1 * vw3 * getShadowMapDepthComp(base_uv, u1, v3, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw2 * vw3 * getShadowMapDepthComp(base_uv, u2, v3, shadowMapSizeInv, cascadeIndex, lightDepth);
	sum += uw3 * vw3 * getShadowMapDepthComp(base_uv, u3, v3, shadowMapSizeInv, cascadeIndex, lightDepth);

	visibility = sum * 1.0 / 2704.0;

	#endif
	// end WZ_SHADOW_FILTER_SIZE checks

#endif
// end WZ_SHADOW_MODE == 1

#if WZ_SHADOW_MODE == 2

	// PCF

	visibility = texture( shadowMap, vec4(pos.xy, cascadeIndex, (pos.z+bias)) );

	#if WZ_SHADOW_FILTER_SIZE >= 2
	const float edgeVal = 0.5+float((WZ_SHADOW_FILTER_SIZE-2)/2);
	const float startVal = -edgeVal;
	const float endVal = edgeVal + 0.5;
	float texelIncrement = 1.0/float(ShadowMapSize);
	const float visibilityIncrement = 0.5 / WZ_SHADOW_FILTER_SIZE;
	for (float y=startVal; y<endVal; y+=1.0)
	{
		for (float x=startVal; x<endVal; x+=1.0)
		{
			visibility -= visibilityIncrement*(1.0-texture( shadowMap, vec4(pos.xy + vec2(x*texelIncrement, y*texelIncrement), cascadeIndex, (pos.z+bias)) ));
		}
	}
	#endif
	// end WZ_SHADOW_FILTER_SIZE >= 2

#endif
// end WZ_SHADOW_MODE == 2

	visibility = clamp(visibility, 0.3, 1.0);
	return visibility;
#endif
}

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

vec4 applyShieldFuzzEffect(vec4 color) {
	float cycle = 0.66 + 0.66 * graphicsCycle;
	vec3 col = vec3(random(vec2(fragPos.x * cycle, fragPos.y * cycle)));
	col.r *= 0.5;
	col.g *= 0.5;
	col.b *= 2.0;
	return vec4(col, color.a / 6.0);
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

	mat3 identityMat = mat3(
		1., 0., 0.,
		0., 1., 0.,
		0., 0., 1.
	);
	// Normals are in view space, we need to get back to world space
	vec3 worldSpaceNormal = -(inverse(ViewMatrix) * vec4(N, 0.f)).xyz;
	light += iterateOverAllPointLights(clipSpaceCoord, fragPos, worldSpaceNormal, normalize(halfVec - lightDir), diffuse, specularMapValue, identityMat);
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
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1.f)
		{
			discard;
		}

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
