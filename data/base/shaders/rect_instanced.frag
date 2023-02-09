// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#ifdef NEWGL
#define FRAGMENT_INPUT in
#else
#define FRAGMENT_INPUT varying
#endif

FRAGMENT_INPUT vec4 colour;

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#ifdef NEWGL
	FragColor = colour;
	#else
	gl_FragColor = colour;
	#endif
}
