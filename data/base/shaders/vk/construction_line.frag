#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelViewMatrix;
	vec4 color;
	vec4 fogColor;
	vec4 fogRange;
};

layout(location = 0) in vec3 posViewSpace;
layout(location = 0) out vec4 FragColor;

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
