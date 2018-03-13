// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform vec4 color;

#if __VERSION__ >= 130
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#if __VERSION__ >= 130
	FragColor = color;
	#else
	gl_FragColor = color;
	#endif
}
