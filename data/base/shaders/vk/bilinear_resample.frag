#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 uvScaleClamp;
};
layout(set = 1, binding = 0) uniform sampler2D sourceTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	// Preserve the full value contract. R8 destinations retain red; RGBA effects
	// retain color and confidence without needing effect-specific resamplers.
	FragColor = texture(sourceTexture, uv);
}
