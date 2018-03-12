uniform sampler2D tex;
uniform sampler2D lightmap_tex;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if __VERSION__ >= 130
in vec2 uv_tex;
in vec2 uv_lightmap;
in float vertexDistance;
#else
varying vec2 uv_tex;
varying vec2 uv_lightmap;
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
	#if __VERSION__ >= 130
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
