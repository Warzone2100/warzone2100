#version 450

// One step up the bloom chain: adds the tent-filtered smaller level onto this one

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 uvScaleClamp; // of the smaller level being read
	vec4 params;       // xy = 1 / its texture size, z = tent radius in its texels
};

layout(set = 1, binding = 0) uniform sampler2D sourceTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

#include "bloom_filters.glsl"

void main()
{
	vec2 uv = texCoords * uvScaleClamp.xy;
	FragColor = vec4(bloomUpsampleTent(uv, params.xy, params.z), 1.0);
}
