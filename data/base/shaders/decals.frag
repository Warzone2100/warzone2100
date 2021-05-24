// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D tex;
uniform sampler2D lightmap_tex;
uniform sampler2D TextureNormal;
uniform sampler2D TextureSpecular;
uniform sampler2D TextureHeight;

// sun light colors/intensity:
uniform vec4 emissiveLight;
uniform vec4 ambientLight;
uniform vec4 diffuseLight;
uniform vec4 specularLight;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv) texture2D(tex,uv)
#endif

#ifdef NEWGL
in vec2 uv_tex;
in vec2 uv_lightmap;
in float vertexDistance;
// Light in tangent space:
in vec3 lightDir;
in vec3 halfVec;
#else
varying vec2 uv_tex;
varying vec2 uv_lightmap;
varying float vertexDistance;
// Light in tangent space:
varying vec3 lightDir;
varying vec3 halfVec;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
#define FragColor gl_FragColor
#endif

void main()
{
	vec3 N = normalize(texture(TextureNormal, uv_tex).xyz * 2.0 - 1.0);
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); // diffuse lighting

	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float angle = acos(dot(H, N));
	float exponent = angle / 0.2;
	exponent = -(exponent * exponent);
	float gaussianTerm = exp(exponent) * float(lambertTerm > 0);
	vec4 gloss = texture(TextureSpecular, uv_tex);

	vec4 texColor = texture(tex, uv_tex);
	vec4 texAlpha = vec4(1,1,1,texColor.a);
	vec4 fragColor = texColor*(ambientLight + diffuseLight*lambertTerm) + texAlpha*specularLight*gloss*gaussianTerm;
	fragColor *= texture(lightmap_tex, uv_lightmap);

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), fogFactor);
	}
	FragColor = fragColor;
}
