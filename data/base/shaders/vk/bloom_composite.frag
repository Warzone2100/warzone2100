#version 450

// Adds the bloom onto the scene (the color mask keeps the scene's alpha)

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 uvScaleClamp; // of the largest bloom level
	vec4 params;       // xy = 1 / its texture size, z = tent radius, w = intensity
	vec4 params2;      // x = 1 / number of levels summed into it
};

layout(set = 1, binding = 0) uniform sampler2D sourceTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

#include "bloom_filters.glsl"

void main()
{
	vec2 uv = texCoords * uvScaleClamp.xy;
	vec3 bloom = bloomUpsampleTent(uv, params.xy, params.z) * params2.x;
	FragColor = vec4(bloom * params.w, 1.0);
}
