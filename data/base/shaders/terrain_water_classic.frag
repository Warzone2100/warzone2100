// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
//

uniform sampler2D lightmap_tex;
uniform sampler2D tex2;

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
in vec2 uvLightmap;
in vec2 uv1;
in vec2 uv2;
in float depth;
in float vertexDistance;
#else
varying vec2 uvLightmap;
varying vec2 uv1;
varying vec2 uv2;
varying float depth;
varying float vertexDistance;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
#define FragColor gl_FragColor
#endif

vec4 main_legacy()
{
	vec4 decal = texture(tex2, uv2, WZ_MIP_LOAD_BIAS);
	vec4 light = texture(lightmap_tex, uvLightmap, 0.f);
	return light * decal;
}

void main()
{
	vec4 fragColor = main_legacy();

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
