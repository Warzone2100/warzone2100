// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D tex;
uniform sampler2D lightmap_tex;
uniform sampler2D TextureNormal; // normal map
uniform sampler2D TextureSpecular; // specular map
uniform sampler2D TextureHeight;

// light colors/intensity:
uniform vec4 emissiveLight;
uniform vec4 ambientLight;
uniform vec4 diffuseLight;
uniform vec4 specularLight;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;
uniform int hasNormalmap; // whether a normal map exists for the model
uniform int hasSpecularmap; // whether a specular map exists for the model
uniform int hasHeightmap;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 color;
in vec2 uv;
in vec2 uvLight;
in float vertexDistance;
// Light in tangent space:
in vec3 lightDir;
in vec3 halfVec;
#else
varying vec4 color;
varying vec2 uv;
varying vec2 uvLight;
varying float vertexDistance;
// Light in tangent space:
varying vec3 lightDir;
varying vec3 halfVec;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#else
#define texture(tex,uv) texture2D(tex,uv)
#endif

void main()
{
	vec3 N;
	if (hasNormalmap != 0) {
		vec3 normalFromMap = texture(TextureNormal, uv).xyz;
		N = normalize(normalFromMap * 2.0 - 1.0);
		N.y = -N.y;
	} else {
		N = vec3(0,0,1); // in tangent space so normal to xy plane
	}
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); // diffuse lighting

	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float angle = acos(dot(H, N));
	float exponent = angle / 0.5; // shininess 0.2 is too small for dry and dirty terrain
	exponent = -(exponent * exponent);
	float gaussianTerm = exp(exponent) * float(lambertTerm > 0);

	vec4 gloss;
	if (hasSpecularmap != 0) {
		gloss = texture(TextureSpecular, uv);
	} else {
		gloss = vec4(vec3(0.1), 1);
	}

	vec4 fragColor = texture(tex, uv)*(ambientLight + diffuseLight*lambertTerm) + specularLight*gloss*gaussianTerm;
	fragColor *= texture(lightmap_tex, uvLight);
	fragColor.a = color.a;

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1)
		{
			discard;
		}

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), clamp(fogFactor, 0.0, 1.0));
	}

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
