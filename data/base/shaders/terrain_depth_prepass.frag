// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#ifdef NEWGL
in vec3 viewNormal;
out vec4 FragColor;
#else
varying vec3 viewNormal;
#endif

void main()
{
	vec3 n = normalize(viewNormal);
	// Alpha = SSAO application weight (1.0 = full terrain AO).
	#ifdef NEWGL
	FragColor = vec4(n * 0.5 + 0.5, 1.0);
	#else
	gl_FragColor = vec4(n * 0.5 + 0.5, 1.0);
	#endif
}
