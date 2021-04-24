// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex1_nm;
uniform sampler2D tex2_nm;
uniform sampler2D tex1_sm;
uniform sampler2D tex2_sm;

// light colors/intensity:
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
in vec2 uv1;
in vec2 uv2;
in float vertexDistance;
// light in modelSpace:
in vec3 lightDir;
in vec3 halfVec;
#else
varying vec2 uv1;
varying vec2 uv2;
varying float vertexDistance;
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
	vec4 fragColor = texture(tex1, uv1) * texture(tex2, uv2);

	vec3 N = texture(tex1_nm, uv1).xzy; // y is up in modelSpace
	if (N == vec3(0,0,0)) {
		N = vec3(0,1,0);
	} else {
		N = normalize(N * 2.0 - 1.0);
	}
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); // diffuse lighting

	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float angle = acos(dot(H, N));
	float exponent = angle / 0.2;
	exponent = -(exponent * exponent);
	float gaussianTerm = exp(exponent) * float(lambertTerm > 0);

	vec4 gloss = texture(tex1_sm, uv1) * texture(tex2_sm, uv2);

	fragColor = fragColor*(ambientLight + diffuseLight*lambertTerm) + specularLight*gloss*gaussianTerm;

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, vec4(1), fogFactor);
	}
	FragColor = fragColor;
}
