uniform sampler2D lightmap_tex;

#if __VERSION__ >= 130
out vec2 uv2;
out vec4 FragColor;
#else
varying vec2 uv2;
// Uses gl_FragColor
#endif

void main()
{
	#if __VERSION__ >= 130
	FragColor = texture(lightmap_tex, uv2);
	#else
	gl_FragColor = texture2D(lightmap_tex, uv2);
	#endif
}
