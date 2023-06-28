// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
//

uniform sampler2D tex;
uniform sampler2D lightmap_tex;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#endif

#ifdef NEWGL
in vec2 uv_tex;
in vec2 uv_lightmap;
in float vertexDistance;
#else
varying vec2 uv_tex;
varying vec2 uv_lightmap;
varying float vertexDistance;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	vec4 fragColor = texture(tex, uv_tex, WZ_MIP_LOAD_BIAS) * vec4(texture(lightmap_tex, uv_lightmap, 0.f).rgb, 1.f);
	
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), fogFactor);
	}

	#ifdef NEWGL
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
