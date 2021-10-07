// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

uniform sampler2D Texture; // diffuse map
uniform sampler2D TextureTcmask; // tcmask
uniform sampler2D TextureNormal; // normal map
uniform sampler2D TextureSpecular; // specular map
uniform vec4 colour; // ?
uniform vec4 teamcolour; // the team colour of the model
uniform int tcmask; // whether a tcmask texture exists for the model
uniform int normalmap; // whether a normal map exists for the model
uniform int specularmap; // whether a specular map exists for the model
uniform int hasTangents; // whether tangents were calculated for model
uniform mat4 NormalMatrix;
uniform bool ecmEffect; // whether ECM special effect is enabled
uniform bool alphaTest;
uniform float graphicsCycle; // a periodically cycling value for special effects

uniform vec4 sceneColor; //emissive light
uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 specular;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in float vertexDistance;
in vec3 normal;
in vec3 lightDir;
in vec3 halfVec;
in vec2 texCoord;
#else
varying float vertexDistance;
varying vec3 normal;
varying vec3 lightDir;
varying vec3 halfVec;
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

	if (alphaTest && (diffuseMap.a <= 0.5))
	{
		discard;
	}

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

	if (lambertTerm > 0.0)
	{
		float vanillaFactor = 0.0; // Classic models shouldn't use diffuse light

		if (specularmap != 0)
		{
			#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
			float specularMapValue = texture(TextureSpecular, texCoord).r;
			#else
			float specularMapValue = texture2D(TextureSpecular, texCoord).r;
			#endif
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
		#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
		float maskAlpha = texture(TextureTcmask, texCoord).r;
		#else
		float maskAlpha = texture2D(TextureTcmask, texCoord).r;
		#endif

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

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
