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
in float vertexDistance;
in vec3 normal;
in vec3 lightDir;
in vec3 halfVec;
in vec2 texCoord;
in mat3 TangentSpaceMatrix;
#else
varying float vertexDistance;
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
	vec4 light = sceneColor;
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); //diffuse light

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
		fragColour = (light + (teamcolour - 0.5) * maskAlpha) * colour;
	}
	else
	{
		fragColour = light * colour;
	}

	if (ecmEffect > 0)
	{
		fragColour.a = 0.66 + 0.66 * graphicsCycle;
	}

	#ifdef NEWGL
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
