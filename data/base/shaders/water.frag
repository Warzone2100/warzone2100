// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
//

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex1_nm;
uniform sampler2D tex2_nm;
uniform sampler2D tex1_sm;
uniform sampler2D tex2_sm;
uniform sampler2D lightmap_tex;

// light colors/intensity:
uniform vec4 emissiveLight;
uniform vec4 ambientLight;
uniform vec4 diffuseLight;
uniform vec4 specularLight;

uniform vec4 fogColor;
uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;

uniform int quality; // 0-classic, 1-bumpmapping

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#endif

#ifdef NEWGL
in vec4 uv1_uv2;
in vec2 uvLightmap;
in float depth;
in float depth2;
in float vertexDistance;
// light in modelSpace:
in vec3 lightDir;
in vec3 halfVec;
#else
varying vec4 uv1_uv2;
varying vec2 uvLightmap;
varying float depth;
varying float depth2;
varying float vertexDistance;
varying vec3 lightDir;
varying vec3 halfVec;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
#define FragColor gl_FragColor
#endif

vec4 main_medium()
{
	vec2 uv1 = uv1_uv2.xy;
	vec2 uv2 = uv1_uv2.zw;
	vec4 fragColor = texture(tex1, uv1, WZ_MIP_LOAD_BIAS);
	float specColor = texture(tex2, uv2, WZ_MIP_LOAD_BIAS).r;
	fragColor *= vec4(specColor, specColor, specColor, 1.0);
	return fragColor;
}

vec4 main_bumpMapping()
{
	vec2 uv1 = uv1_uv2.xy;
	vec2 uv2 = uv1_uv2.zw;

	vec3 N1 = texture(tex1_nm, uv2, WZ_MIP_LOAD_BIAS).xzy; // y is up in modelSpace
	vec3 N2 = texture(tex2_nm, uv1, WZ_MIP_LOAD_BIAS).xzy;
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
	float gloss = texture(tex1_sm, uv1, WZ_MIP_LOAD_BIAS).r * texture(tex2_sm, uv2, WZ_MIP_LOAD_BIAS).r;
	vec3 H = normalize(halfVec);
	float exponent = acos(dot(H, N)) / (gloss + 0.05);
	float gaussianTerm = exp(-(exponent * exponent));

	vec4 fragColor = (texture(tex1, uv1, WZ_MIP_LOAD_BIAS)+texture(tex2, uv2, WZ_MIP_LOAD_BIAS)) * (gloss+vec4(0.08,0.13,0.15,1.0));
	fragColor = fragColor*(ambientLight+diffuseLight*lambertTerm) + specularLight*(1-gloss)*gaussianTerm*vec4(1.0,0.843,0.686,1.0);
	vec4 light = texture(lightmap_tex, uvLightmap, 0.f);
	return light * fragColor;
}

void main()
{
	vec4 fragColor;
	if (quality == 2) {
		fragColor = main_bumpMapping();
		fragColor = mix(fragColor, fragColor-depth*0.0007, depth*0.0009);
		fragColor.a = mix(0.25, 1.0, depth2*0.005);
	} else {
		fragColor = main_medium();
	}

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, fogColor, fogFactor);
	}

	FragColor = fragColor;
}
