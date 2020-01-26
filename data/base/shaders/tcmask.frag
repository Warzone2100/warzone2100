// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

uniform sampler2D Texture; // diffuse
uniform sampler2D TextureTcmask; // tcmask
uniform sampler2D TextureNormal; // normal map
uniform sampler2D TextureSpecular; // specular map
uniform vec4 colour;
uniform vec4 teamcolour; // the team colour of the model
uniform int tcmask; // whether a tcmask texture exists for the model
uniform int normalmap; // whether a normal map exists for the model
uniform int specularmap; // whether a specular map exists for the model
uniform int hasTangents; // whether tangents were calculated for model
uniform mat4 NormalMatrix;
uniform bool ecmEffect; // whether ECM special effect is enabled
uniform bool alphaTest;
uniform float graphicsCycle; // a periodically cycling value for special effects

uniform vec4 sceneColor;
uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 specular;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in float vertexDistance;
in vec3 normal, lightDir, halfVec;
in vec2 texCoord;
#else
varying float vertexDistance;
varying vec3 normal, lightDir, halfVec;
varying vec2 texCoord;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	vec4 diffuseMap = texture(Texture, texCoord);
	#else
	vec4 diffuseMap = texture2D(Texture, texCoord);
	#endif

	// Normal map implementations
	vec3 N = normal;
	if (normalmap != 0)
	{
		#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
		vec3 normalFromMap = texture(TextureNormal, texCoord).xyz;
		#else
		vec3 normalFromMap = texture2D(TextureNormal, texCoord).xyz;
		#endif

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
			#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
			vec4 specularFromMap = texture(TextureSpecular, texCoord);
			#else
			vec4 specularFromMap = texture2D(TextureSpecular, texCoord);
			#endif

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
		#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
		vec4 mask = texture(TextureTcmask, texCoord);
		#else
		vec4 mask = texture2D(TextureTcmask, texCoord);
		#endif

		// Apply color using grain merge with tcmask
		fragColour = (light + (teamcolour - 0.5) * mask.a) * colour;
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
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColour = mix(fogColor, fragColour, fogFactor);
	}

	if (!alphaTest && (diffuseMap.a <= 0.5))
	{
		discard;
	}

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
