// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex1_nm;
uniform sampler2D tex2_nm;
uniform sampler2D tex1_sm;
uniform sampler2D tex2_sm;
uniform sampler2D tex1_hm;
uniform sampler2D tex2_hm;

// light colors/intensity:
uniform vec4 emissiveLight;
uniform vec4 ambientLight;
uniform vec4 diffuseLight;
uniform vec4 specularLight;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

uniform int quality; // 0-classic, 1-bumpmapping

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv) texture2D(tex,uv)
#endif

#ifdef NEWGL
in vec2 uv1;
in vec2 uv2;
in float depth;
in float vertexDistance;
// light in modelSpace:
in vec3 lightDir;
in vec3 halfVec;
#else
varying vec2 uv1;
varying vec2 uv2;
varying float depth;
varying float vertexDistance;
varying vec3 lightDir;
varying vec3 halfVec;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
#define FragColor gl_FragColor
#endif

vec4 main_classic()
{
	vec4 fragColor = texture(tex1, uv1);
	float specColor = texture(tex2, uv2).r;
	fragColor *= vec4(specColor, specColor, specColor, 1.0);
	return fragColor;
}

vec4 main_bumpMapping()
{
	vec4 fragColor = texture(tex1, uv1) * texture(tex2, uv2);

	vec3 N1 = texture(tex1_nm, uv1).xzy; // y is up in modelSpace
	vec3 N2 = texture(tex2_nm, uv2).xzy;
	vec3 N; //use overlay blending to mix normal maps properly
	N.x = N1.x < 0.5 ? (2 * N1.x * N2.x) : (1 - 2 * (1 - N1.x) * (1 - N2.x));
	N.z = N1.z < 0.5 ? (2 * N1.z * N2.z) : (1 - 2 * (1 - N1.z) * (1 - N2.z));
	N.y = N1.y < 0.5 ? (2 * N1.y * N2.y) : (1 - 2 * (1 - N1.y) * (1 - N2.y));
	if (N == vec3(0,0,0)) {
		N = vec3(0,1,0);
	} else {
		N = normalize(N * 2.0 - 1.0);
	}

	float lambertTerm = max(dot(N, lightDir), 0.0); // diffuse lighting

	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float exponent = acos(dot(H, N)) / 0.95;
	float gaussianTerm = exp(-(exponent * exponent));

	vec4 gloss = vec4(vec3(texture(tex1_sm, uv1).r), 1) * vec4(vec3(texture(tex2_sm, uv2).r), 1);

	return fragColor*(ambientLight + diffuseLight*lambertTerm) + specularLight*gloss*gaussianTerm;
}

void main()
{
	vec4 fragColor;
	if (quality == 0) {
		fragColor = main_classic();
	} else {
		fragColor = main_bumpMapping();
	}

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
