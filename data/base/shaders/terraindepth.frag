// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

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
