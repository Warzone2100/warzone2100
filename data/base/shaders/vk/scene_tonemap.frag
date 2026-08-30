#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 uvScaleClamp;
	float tonemapKnee;
	float encodeOutput;
};

layout(set = 1, binding = 0) uniform sampler2D sceneTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

#include "srgb.glsl"

// Leaves values below the knee unchanged and rolls values above it off smoothly toward 1
vec3 tonemapShoulder(vec3 color, float knee)
{
	vec3 excess = max(color - vec3(knee), vec3(0.0));
	float headroom = max(1.0 - knee, 1e-4);
	vec3 shoulder = headroom * (vec3(1.0) - exp(-excess / headroom));
	return min(color, vec3(knee)) + shoulder;
}

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	vec4 scene = texture(sceneTexture, uv);

	vec3 result = tonemapShoulder(scene.rgb, tonemapKnee);
	if (encodeOutput > 0.5)
	{
		// Linear lighting is on, so encode the scene for display
		result = wzLinearToSrgb(result);
	}

	FragColor = vec4(result, scene.a);
}
