// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D tex;
uniform sampler2D lightmap_tex;
uniform sampler2D TextureNormal; // normal map
uniform sampler2D TextureSpecular; // specular map

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;
uniform int hasNormalmap; // whether a normal map exists for the model
uniform int hasSpecularmap; // whether a specular map exists for the model

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 color;
in vec2 uv1;
in vec2 uv2;
in float vertexDistance;
// Light in tangent space:
in vec3 lightDir;
in vec3 halfVec;
#else
varying vec4 color;
varying vec2 uv1;
varying vec2 uv2;
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
	vec4 mask = color * texture(lightmap_tex, uv2);
	vec4 fragColor = mask * texture(tex, uv1);

	vec3 N;
	if (hasNormalmap != 0) {
		vec3 normalFromMap = texture(TextureNormal, uv1).xyz;
		N = normalize(normalFromMap * 2.0 - 1.0);
	} else {
		N = vec3(0,0,1); // in tangent space so normal to xy plane
	}
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); // diffuse lighting

	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float angle = acos(dot(H, N));
	float exponent = angle / 0.2;
	exponent = -(exponent * exponent);
	vec4 gaussianTerm = mask*exp(exponent);
	if (hasSpecularmap != 0) {
		gaussianTerm *= texture(TextureSpecular, uv1);
	} else {
		gaussianTerm *= 0.4;
	}

	fragColor = fragColor*(lambertTerm*0.4 + 0.3) + gaussianTerm;

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1.f)
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
