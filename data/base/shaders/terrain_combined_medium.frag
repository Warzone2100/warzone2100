// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// Texture arrays are supported in:
// 1. core OpenGL 3.0+ (and earlier with EXT_texture_array)
//#if (!defined(GL_ES) && (__VERSION__ < 130))
//#extension GL_EXT_texture_array : require // only enable this on OpenGL < 3.0
//#endif
// 2. OpenGL ES 3.0+
#if (defined(GL_ES) && (__VERSION__ < 300))
#error "Unsupported version of GLES"
#endif

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
#define WZ_EXTRA_SHADOW_TAPS 4
#define WZ_SHADOW_CASCADES_COUNT 3
//

#define WZ_MAX_SHADOW_CASCADES 3

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#define texture2DArray(tex,coord,bias) texture(tex,coord,bias)
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#endif

uniform sampler2D lightmap_tex;

// ground texture arrays. layer = ground type
uniform sampler2DArray groundTex;
uniform sampler2DArray groundNormal;
uniform sampler2DArray groundSpecular;
uniform sampler2DArray groundHeight;

// array of scales for ground textures, encoded in mat4. scale_i = groundScale[i/4][i%4]
uniform mat4 groundScale;

// decal texture arrays. layer = decal tile
uniform sampler2DArray decalTex;
uniform sampler2DArray decalNormal;
uniform sampler2DArray decalSpecular;
uniform sampler2DArray decalHeight;

// shadow map
uniform sampler2DArrayShadow shadowMap;

uniform mat4 ViewMatrix;
uniform mat4 ShadowMapMVPMatrix[WZ_MAX_SHADOW_CASCADES];
uniform vec4 ShadowMapCascadeSplits;
uniform int ShadowMapSize;

// sun light colors/intensity:
uniform vec4 emissiveLight;
uniform vec4 ambientLight;
uniform vec4 diffuseLight;
uniform vec4 specularLight;

uniform vec4 sunPos; // in modelSpace, normalized

// fog
uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

in vec2 uvLightmap;
in vec2 uvDecal;
in vec2 uvGround;
in float vertexDistance;
flat in int tile;
flat in uvec4 fgrounds;
in vec4 fgroundWeights;
// Light in tangent space:
in vec3 groundLightDir;
in vec3 groundHalfVec;
in mat2 decal2groundMat2;
// For Shadows
in vec3 fragPos;
in vec3 fragNormal;

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

float getShadowVisibility()
{
#if WZ_EXTRA_SHADOW_TAPS > 0

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

	vec3 normal = normalize(fragNormal);
	vec3 lightDir = normalize(normalize(sunPos.xyz) - normalize(fragPos));
//	float bias = max(0.0002 * (1.0 - dot(normal, lightDir)), 0.0002);
	float bias = 0.0001f;

	float visibility = texture( shadowMap, vec4(pos.xy, cascadeIndex, (pos.z+bias)) );

	// PCF
	const float edgeVal = 0.5+float((WZ_EXTRA_SHADOW_TAPS-1)/2);
	const float startVal = -edgeVal;
	const float endVal = edgeVal + 0.5;
	float texelIncrement = 1.0/float(ShadowMapSize);
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
#else
	return 1.0;
#endif
}

vec3 getGroundUv(int i) {
	uint groundNo = fgrounds[i];
	return vec3(uvGround * groundScale[groundNo/4u][groundNo%4u], groundNo);
}

vec3 getGround(int i) {
	return texture2DArray(groundTex, getGroundUv(i), WZ_MIP_LOAD_BIAS).rgb * fgroundWeights[i];
}

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

vec4 main_medium() {
	vec3 ground = getGround(0) + getGround(1) + getGround(2) + getGround(3);
	vec4 decal = tile >= 0 ? texture2DArray(decalTex, vec3(uvDecal, tile), WZ_MIP_LOAD_BIAS) : vec4(0.f);
	float visibility = getShadowVisibility();

	vec3 L = normalize(groundLightDir);
	vec3 N = vec3(0.f,0.f,1.f);
	float lambertTerm = max(dot(N, L), 0.0); // diffuse lighting
	vec4 lightmap_vec4 = texture(lightmap_tex, uvLightmap, 0.f);
	vec4 light = (visibility*diffuseLight*0.75*lambertTerm + ambientLight*0.25) * lightmap_vec4.a; // ... * tile brightness / ambient occlusion (stored in lightmap.a)
	light.rgb = blendAddEffectLighting(light.rgb, (lightmap_vec4.rgb / 1.5f)); // additive color (from environmental point lights / effects)
	light.a = 1.f;

	return light * vec4((1.f - decal.a) * ground + decal.a * decal.rgb, 1.f);
}

void main()
{
	vec4 fragColor = main_medium();

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), fogFactor);
	}

	#ifdef NEWGL
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
