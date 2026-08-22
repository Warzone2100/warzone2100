// Version directive is set by Warzone when loading the shader.

layout(std140) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelViewMatrix;
	vec4 color;
	vec4 fogColor;
	vec4 fogRange;
};

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec3 posViewSpace;
out vec4 FragColor;
#else
varying vec3 posViewSpace;
#define FragColor gl_FragColor
#endif

#include "distance_fog.glsl"

void main()
{
	vec4 result = color;
	if (fogRange.z > 0.5)
	{
		float fogAmount = wzDistanceFogAmount(length(posViewSpace), fogRange.x, fogRange.y);
		result.rgb = wzApplyForwardFog(result.rgb, result.a, fogAmount, fogColor.rgb, WZ_FOG_OUTPUT_ADDITIVE);
	}
	FragColor = result;
}
