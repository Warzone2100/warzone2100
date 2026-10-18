#version 450

#include "srgb.glsl"

layout(std140, set = 0, binding = 0) uniform cbuffer {
	float ssaoIntensity;
	float linearLighting;
	float padding1;
	float padding2;
	vec4 sceneUvScaleClamp;
	vec4 aoUvScaleClamp;
};

layout(set = 1, binding = 0) uniform sampler2D sceneTexture;
layout(set = 1, binding = 1) uniform sampler2D ssaoTexture;
layout(set = 1, binding = 2) uniform sampler2D prepassNormals;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec2 sceneUv = clamp(texCoords * sceneUvScaleClamp.xy, vec2(0.0), sceneUvScaleClamp.zw);
	vec2 aoUv = clamp(texCoords * aoUvScaleClamp.xy, vec2(0.0), aoUvScaleClamp.zw);
	vec3 scene = texture(sceneTexture, sceneUv).rgb;
	float ao = texture(ssaoTexture, aoUv).r;
	// A is SSAO weight (1 = apply). Water prepass writes 0, so lakes get no AO.
	float weight = texture(prepassNormals, sceneUv).a;
	float aoFactor = mix(1.0, ao, ssaoIntensity * weight);
	vec3 litAo;
	if (linearLighting > 0.5)
	{
		// The AO strength was tuned for display values, so it darkens in display space
		litAo = wzSrgbToLinear(wzLinearToSrgb(scene) * aoFactor);
	}
	else
	{
		litAo = scene * aoFactor;
	}
	FragColor = vec4(litAo, 1.0);
}
