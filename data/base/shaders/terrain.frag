uniform sampler2D tex;
uniform sampler2D lightmap_tex;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if __VERSION__ >= 130
in vec4 color;
in vec2 uv1;
in vec2 uv2;
in float vertexDistance;
#else
varying vec4 color;
varying vec2 uv1;
varying vec2 uv2;
varying float vertexDistance;
#endif

#if __VERSION__ >= 130
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#if __VERSION__ >= 130
	vec4 fragColor = color * texture(tex, uv1) * texture(lightmap_tex, uv2);
	#else
	vec4 fragColor = color * texture2D(tex, uv1) * texture2D(lightmap_tex, uv2);
	#endif
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fogColor, fragColor, fogFactor);
	}

	#if __VERSION__ >= 130
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
