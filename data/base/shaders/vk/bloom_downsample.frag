#version 450

// One step down the bloom chain, to half the source size

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 uvScaleClamp; // of the source
	vec4 params;       // xy = 1 / source size, z = 1 for the 9-tap fit, w = threshold (first step only)
};

layout(set = 1, binding = 0) uniform sampler2D sourceTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

#include "bloom_filters.glsl"

void main()
{
	vec2 uv = texCoords * uvScaleClamp.xy;
	vec3 c = (params.z > 0.5) ? bloomDownsample9(uv, params.xy) : bloomDownsample13(uv, params.xy, params.w);
	FragColor = vec4(c, 1.0);
}
