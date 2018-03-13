// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if __VERSION__ >= 130
in vec4 vColour;
#else
varying vec4 vColour;
#endif

#if __VERSION__ >= 130
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#if __VERSION__ >= 130
	FragColor = vColour;
	#else
	gl_FragColor = vColour;
	#endif
}
