// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
//

uniform sampler2D tex1;
uniform sampler2D tex2;
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

vec4 main_medium()
{
	vec2 uv1 = uv1_uv2.xy;
	vec2 uv2 = uv1_uv2.zw;
	vec4 fragColor = texture(tex1, uv1, WZ_MIP_LOAD_BIAS);
	float specColor = texture(tex2, uv2, WZ_MIP_LOAD_BIAS).r;
	fragColor *= vec4(specColor, specColor, specColor, 1.0);
	return fragColor;
}

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

void main()
{
	vec4 fragColor = main_medium();

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if (fogFactor >= 1.f)
		{
			discard;
		}

		fogFactor = 1.0 - clamp(fogFactor, 0.0, 1.0);
		fragColor.rgb *= fogFactor; // premultiply by fogFactor as alpha
		fragColor.a = fogFactor;
	}

	FragColor = fragColor;
}
