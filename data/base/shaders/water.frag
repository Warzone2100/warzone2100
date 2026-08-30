// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#include "terrain_water.glsl"
#include "srgb.glsl"
//

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D lightmap_tex;

// light colors/intensity:


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
	if (linearLighting != 0)
	{
		// This water multiplies what is already drawn by color * mask + (1 - alpha), tuned for display values.
		// Under linear lighting the whole multiplier, alpha included, is computed in display space,
		// so the blend itself does no math in linear.
		vec3 multiplier = wzLinearToSrgb(fragColor.rgb) * specColor + vec3(1.0 - fragColor.a);
		return vec4(wzSrgbToLinear(multiplier), 1.0);
	}
	fragColor *= vec4(specColor, specColor, specColor, 1.0);
	return fragColor;
}

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

void main()
{
	vec4 fragColor = main_medium();

	FragColor = fragColor;
}
