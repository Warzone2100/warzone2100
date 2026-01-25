// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
#define WZ_MIP_LOAD_BIAS 0.f
//

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

//#ifdef NEWGL
//out vec4 FragColor;
//#else
//// Uses gl_FragColor
//#endif

void main()
{
//	#ifdef NEWGL
//	FragColor = fragColor;
//	#else
//	gl_FragColor = fragColor;
//	#endif
}
