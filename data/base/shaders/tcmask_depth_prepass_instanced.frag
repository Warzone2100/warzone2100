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

#define MESH_SSAO_WEIGHT 1.0

void main()
{
	vec3 n = normalize(viewNormal);
	vec3 encoded = n * 0.5 + 0.5;
	#ifdef NEWGL
	FragColor = vec4(encoded, MESH_SSAO_WEIGHT);
	#else
	gl_FragColor = vec4(encoded, MESH_SSAO_WEIGHT);
	#endif
}
