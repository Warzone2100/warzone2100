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
