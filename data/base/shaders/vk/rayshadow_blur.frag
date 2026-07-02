#version 460

// Depth-aware separable Gaussian blur for the ray-traced shadow/AO mask (RG).
// Run twice (horizontal then vertical) to soften the low-sample-count trace results.

layout(set = 0, binding = 0) uniform sampler2D maskTexture;
layout(set = 0, binding = 1) uniform sampler2D depthTexture;

layout(push_constant) uniform PushConstants
{
	vec4 dirAndInvSize; // xy = blur direction (in texels), zw = 1 / mask texture size
	vec4 params;        // x = relative view-depth rejection threshold (e.g. 0.08 = 8%)
};

// camera near/far planes (must match pie_PerspectiveGet, lib/ivis_opengl/piematrix.cpp)
const float WZ_ZNEAR = 330.0;
const float WZ_ZFAR = 45000.0;

float linearizeDepth(float d)
{
	// GL-convention depth remapped by the engine's z=(z+w)/2 fixup
	float zNdc = 2.0 * d - 1.0;
	return (2.0 * WZ_ZNEAR * WZ_ZFAR) / (WZ_ZFAR + WZ_ZNEAR - zNdc * (WZ_ZFAR - WZ_ZNEAR));
}

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec2 result;

const float WEIGHTS[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);
const float OFFSETS[3] = float[](0.0, 1.3846153846, 3.2307692308);

// the camera depth prepass is rendered without the engine's Vulkan y-flip; flip v to match
// the (screen-oriented) mask
float sampleSceneDepth(vec2 uv)
{
	return texture(depthTexture, vec2(uv.x, 1.0 - uv.y)).r;
}

void main()
{
	vec2 texelDir = dirAndInvSize.xy * dirAndInvSize.zw;
	float centerZ = linearizeDepth(sampleSceneDepth(texCoords));
	vec2 total = texture(maskTexture, texCoords).rg * WEIGHTS[0];
	float totalWeight = WEIGHTS[0];
	for (int i = 1; i < 3; ++i)
	{
		for (int s = -1; s <= 1; s += 2)
		{
			vec2 uv = texCoords + texelDir * (OFFSETS[i] * float(s));
			float sampleZ = linearizeDepth(sampleSceneDepth(uv));
			// reject samples across depth discontinuities (silhouettes) - relative view-space
			// threshold so smooth slopes keep full weights at any distance
			float w = WEIGHTS[i] * max(0.0, 1.0 - abs(sampleZ - centerZ) / (params.x * centerZ));
			total += texture(maskTexture, uv).rg * w;
			totalWeight += w;
		}
	}
	result = total / max(totalWeight, 0.0001);
}
