// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#ifdef NEWGL
in vec3 worldPos;
in vec3 centerRadius;
out vec4 FragColor;
#else
varying vec3 worldPos;
varying vec3 centerRadius;
#endif

void main()
{
	float R = max(centerRadius.z, 1e-3);
	float d = length(worldPos.xz - centerRadius.xy);
	float sdf = d - R;
	if (sdf > 0.25 * R)
	{
		discard;
	}
	float encoded = 0.5 + 0.5 * clamp(sdf / R, -1.0, 1.0);
	#ifdef NEWGL
	FragColor = vec4(encoded, encoded, encoded, 1.0);
	#else
	gl_FragColor = vec4(encoded, encoded, encoded, 1.0);
	#endif
}
