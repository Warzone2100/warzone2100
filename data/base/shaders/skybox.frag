// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform vec4 color;
uniform sampler2D theTexture;

uniform int fog_enabled;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec2 uv;
in vec4 fog;
#else
varying vec2 uv;
varying vec4 fog;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	vec4 resultingColor = texture(theTexture, uv) * color;
	#else
	vec4 resultingColor = texture2D(theTexture, uv) * color;
	#endif

	if(fog_enabled > 0)
	{
		resultingColor = mix(resultingColor, vec4(fog.xyz, 1), fog.w);
	}

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = resultingColor;
	#else
	gl_FragColor = resultingColor;
	#endif
}
