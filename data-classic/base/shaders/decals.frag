// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D tex;
uniform sampler2D lightmap_tex;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec2 uv_tex;
in vec2 uv_lightmap;
in float vertexDistance;
#else
varying vec2 uv_tex;
varying vec2 uv_lightmap;
varying float vertexDistance;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	vec4 fragColor = texture(tex, uv_tex) * texture(lightmap_tex, uv_lightmap);
	#else
	vec4 fragColor = texture2D(tex, uv_tex) * texture2D(lightmap_tex, uv_lightmap);
	#endif
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fogColor, fragColor, fogFactor);
	}
	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
