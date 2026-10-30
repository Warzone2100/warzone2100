#version 450

// One step down the bloom chain, to half the source size

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 uvScaleClamp; // of the source
	vec4 params;       // xy = 1 / source texture size, w = threshold (first step only, else 0)
};

layout(set = 1, binding = 0) uniform sampler2D sourceTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

#include "bloom_filters.glsl"

void main()
{
	vec2 uv = texCoords * uvScaleClamp.xy;
	FragColor = vec4(bloomDownsample13(uv, params.xy, params.w), 1.0);
}
