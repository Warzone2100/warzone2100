// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
#define WZ_EXTRA_SHADOW_TAPS 4
#define WZ_SHADOW_CASCADES_COUNT 3
//

#define WZ_MAX_SHADOW_CASCADES 3

uniform sampler2D Texture; // diffuse map
uniform sampler2D TextureTcmask; // tcmask
uniform sampler2D TextureNormal; // normal map
uniform sampler2D TextureSpecular; // specular map
uniform sampler2DArrayShadow shadowMap; // shadow map

uniform mat4 ViewMatrix;

uniform int tcmask; // whether a tcmask texture exists for the model
uniform int normalmap; // whether a normal map exists for the model
uniform int specularmap; // whether a specular map exists for the model
uniform int hasTangents; // whether tangents were calculated for model
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

// For Shadows
FRAGMENT_INPUT vec3 fragPos;
FRAGMENT_INPUT vec3 fragNormal;

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

	float bias = 0.0002f;

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

		if(fogFactor > 1.f)
		{
			discard;
		}

		// Return fragment color
		fragColour = mix(fragColour, vec4(fogColor.xyz, fragColour.w), clamp(fogFactor, 0.0, 1.0));
	}

	#ifdef NEWGL
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
