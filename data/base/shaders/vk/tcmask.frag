#version 450
//#pragma debug(on)

layout(set = 1, binding = 0) uniform sampler2D Texture; // diffuse
layout(set = 1, binding = 1) uniform sampler2D TextureTcmask; // tcmask
layout(set = 1, binding = 2) uniform sampler2D TextureNormal; // normal map
layout(set = 1, binding = 3) uniform sampler2D TextureSpecular; // specular map

layout(std140, set = 0, binding = 0) uniform cbuffer
{
	vec4 colour;
	vec4 teamcolour; // the team colour of the model
	float stretch;
	int tcmask; // whether a tcmask texture exists for the model
	int fogEnabled; // whether fog is enabled
	int normalmap; // whether a normal map exists for the model
	int specularmap; // whether a specular map exists for the model
	int ecmEffect; // whether ECM special effect is enabled
	int alphaTest;
	float graphicsCycle; // a periodically cycling value for special effects
	mat4 ModelViewMatrix;
	mat4 ModelViewProjectionMatrix;
	mat4 NormalMatrix;
	vec4 lightPosition;
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 fogColor;
	float fogEnd;
	float fogStart;
	int hasTangents;
};

layout(location  = 0) in float vertexDistance;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 lightDir;
layout(location = 3) in vec3 halfVec;
layout(location = 4) in vec2 texCoord;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 diffuseMap = texture(Texture, texCoord);

	if ((alphaTest != 0) && (diffuseMap.a <= 0.5))
	{
		discard;
	}

	// Normal map implementations
	vec3 N = normal;
	if (normalmap != 0)
	{
		vec3 normalFromMap = texture(TextureNormal, texCoord).xyz;

		// Complete replace normal with new value
		N = normalFromMap.xzy * 2.0 - 1.0;

		// To match wz's light
		N.y = -N.y;

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
	float lambertTerm = max(dot(N, L), 0.0);

	if (lambertTerm > 0.0)
	{
		// Vanilla models shouldn't use diffuse light
		float vanillaFactor = 0.0;

		if (specularmap != 0)
		{
			vec4 specularFromMap = texture(TextureSpecular, texCoord);

			// Gaussian specular term computation
			vec3 H = normalize(halfVec);
			float angle = acos(dot(H, N));
			float exponent = angle / 0.2;
			exponent = -(exponent * exponent);
			float gaussianTerm = exp(exponent);

			light += specular * gaussianTerm * lambertTerm * specularFromMap;

			// Neutralize factor for spec map
			vanillaFactor = 1.0;
		}

		light += diffuse * lambertTerm * diffuseMap * vanillaFactor;
	}
	// NOTE: this doubled for non-spec map case to keep results similar to old shader
	// We rely on specularmap to be either 1 or 0 to avoid adding another if
	light += ambient * diffuseMap * (1.0 + (1.0 - float(specularmap)));

	vec4 fragColour;
	if (tcmask != 0)
	{
		// Get mask for team colors from texture
		vec4 mask = texture(TextureTcmask, texCoord);

		// Apply color using grain merge with tcmask
		fragColour = (light + (teamcolour - 0.5) * mask.a) * colour;
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
		fragColour = mix(fogColor, fragColour, fogFactor);
	}

	FragColor = fragColour;
}
