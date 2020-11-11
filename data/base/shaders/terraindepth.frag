// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D lightmap_tex;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec2 uv2;
in float vertexDistance;
#else
varying vec2 uv2;
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
	vec4 fragColor = texture(lightmap_tex, uv2);
	#else
	vec4 fragColor = texture2D(lightmap_tex, uv2);
	#endif
	
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1)
		{
			discard;
		}

		// Return fragment color
		fragColor = fragColor;
	}

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
