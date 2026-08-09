#version 450
//#pragma debug(on)

layout(set = 3, binding = 0) uniform sampler2D Texture; // diffuse
layout(set = 3, binding = 1) uniform sampler2D TextureTcmask; // tcmask
layout(set = 3, binding = 2) uniform sampler2D TextureNormal; // normal map
layout(set = 3, binding = 3) uniform sampler2D TextureSpecular; // specular map

layout(std140, set = 0, binding = 0) uniform globaluniforms
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ShadowMapMVPMatrix;
	vec4 lightPosition;
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 fogColor;
	float fogEnd;
	float fogStart;
	float graphicsCycle;
	int fogEnabled;
	float WZ_MIP_LOAD_BIAS;
};

layout(std140, set = 1, binding = 0) uniform meshuniforms
{
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
};

layout(std140, set = 2, binding = 0) uniform instanceuniforms
{
	mat4 ModelViewMatrix;
	mat4 NormalMatrix;
	vec4 colour;
	vec4 teamcolour;
	float stretch;
	float animFrameNumber;
	int ecmEffect;
	int alphaTest;
};

layout(location  = 0) in float vertexDistance;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 lightDir;
layout(location = 3) in vec3 halfVec;
layout(location = 4) in vec2 texCoord;

layout(location = 0) out vec4 FragColor;

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

		if (hasTangents != 0)
		{
			// Tangent-space normal map, in the conventional (T, B, N) basis.
			// No compensating negation: the vertex shader now negates the light
			// exactly as tcmask_instanced.vert does, so this is identical to
			// that shader's "TangentSpaceMatrix * (normalFromMap.rgb * 2 - 1)",
			// expressed in tangent space rather than world space.
			N = normalFromMap.rgb * 2.0 - 1.0;
		}
		else
		{
			// Object-space normal map. This path goes through NormalMatrix and
			// never touches TangentSpaceMatrix, so the (T, N, B) -> (T, B, N)
			// reorder does not cancel the .xzy swizzle here the way it does
			// above - it has to stay, as it does in tcmask_instanced.frag.
			vec3 nObjectSpace = normalFromMap.xzy * 2.0 - 1.0;
			N = (NormalMatrix * vec4(-nObjectSpace.x, nObjectSpace.y, -nObjectSpace.z, 0.0)).xyz;
		}
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
