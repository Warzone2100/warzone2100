// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
//

uniform sampler2DArray tex;
uniform sampler2DArray tex_nm;
uniform sampler2DArray tex_sm;
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

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#define FRAGMENT_INPUT in
#define texture2DArray(tex,coord,bias) texture(tex,coord,bias)
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#define FRAGMENT_INPUT varying
#endif

FRAGMENT_INPUT vec4 uv1_uv2;
FRAGMENT_INPUT vec2 uvLightmap;
FRAGMENT_INPUT float depth;
FRAGMENT_INPUT float depth2;
FRAGMENT_INPUT float vertexDistance;
// light in modelSpace:
FRAGMENT_INPUT vec3 lightDir;
FRAGMENT_INPUT vec3 halfVec;

#ifdef NEWGL
out vec4 FragColor;
#else
#define FragColor gl_FragColor
#endif

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

vec4 main_bumpMapping()
{
	vec2 uv1 = uv1_uv2.xy;
	vec2 uv2 = uv1_uv2.zw;

	vec3 N1 = texture2DArray(tex_nm, vec3(uv2, 0.f), WZ_MIP_LOAD_BIAS).xzy; // y is up in modelSpace
	vec3 N2 = texture2DArray(tex_nm, vec3(uv1, 1.f), WZ_MIP_LOAD_BIAS).xzy;
	//use overlay blending to mix normal maps properly
	bvec3 computedN_select = lessThan(N1, vec3(0.5));
	vec3 computedN_multiply = 2.f * N1 * N2;
	vec3 computedN_screen = vec3(1.f) - 2.f * (vec3(1.f) - N1) * (vec3(1.f) - N2);
	vec3 N = mix(computedN_screen, computedN_multiply, vec3(computedN_select));

	N = mix(normalize(N * 2.f - 1.f), vec3(0.f,1.f,0.f), vec3(float(N == vec3(0.f,0.f,0.f))));

	float lambertTerm = max(dot(N, lightDir), 0.0); // diffuse lighting

	// Gaussian specular term computation
	float gloss = texture2DArray(tex_sm, vec3(uv1, 0.f), WZ_MIP_LOAD_BIAS).r * texture2DArray(tex_sm, vec3(uv2, 1.f), WZ_MIP_LOAD_BIAS).r;
	vec3 H = normalize(halfVec);
	float exponent = acos(dot(H, N)) / (gloss + 0.05);
	float gaussianTerm = exp(-(exponent * exponent));

	vec4 fragColor = (texture2DArray(tex, vec3(uv1, 0.f), WZ_MIP_LOAD_BIAS)+texture2DArray(tex, vec3(uv2, 1.f), WZ_MIP_LOAD_BIAS)) * (gloss+vec4(0.08,0.13,0.15,1.0));
	fragColor = fragColor*(ambientLight+diffuseLight*lambertTerm) + specularLight*(1.0-gloss)*gaussianTerm*vec4(1.0,0.843,0.686,1.0);
	vec4 lightmap_vec4 = texture(lightmap_tex, uvLightmap, 0.f);
	vec4 color = fragColor * vec4(vec3(lightmap_vec4.a), 1.f); // ... * tile brightness / ambient occlusion (stored in lightmap.a);
	color.rgb = blendAddEffectLighting(color.rgb, (lightmap_vec4.rgb / 1.5f)); // additive color (from environmental point lights / effects)
	return color;
}

void main()
{
	vec4 fragColor = main_bumpMapping();
	fragColor = mix(fragColor, fragColor-depth*0.0007, depth*0.0009);
	fragColor.a = mix(0.25, 1.0, depth2*0.005);

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
