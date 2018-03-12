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
