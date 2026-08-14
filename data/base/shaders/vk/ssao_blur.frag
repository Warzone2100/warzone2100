#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec2 blurDirection;
	float depthSigma;
	float tapPairs;
	vec4 uvScaleClamp;
};

layout(set = 1, binding = 0) uniform sampler2D occlusionTexture;
layout(set = 1, binding = 1) uniform sampler2D depthTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

const float SKY_DEPTH_THRESHOLD = 0.9999;
// Separable 9-tap Gaussian weights (center + 4 pairs).
const float WEIGHT0 = 0.227027;
const float WEIGHT1 = 0.1945946;
const float WEIGHT2 = 0.1216216;
const float WEIGHT3 = 0.054054;
const float WEIGHT4 = 0.016216;

float depthWeight(float centerDepth, float sampleDepth, float sigma)
{
	float d = centerDepth - sampleDepth;
	float s = max(sigma, 1e-6);
	return exp(-(d * d) / (2.0 * s * s));
}

void accumulateTap(inout float result, inout float weightSum, vec2 sampleUV, float centerDepth, float spatialWeight)
{
	sampleUV = clamp(sampleUV, vec2(0.0), uvScaleClamp.zw);
	float sampleDepth = texture(depthTexture, sampleUV).r;
	if (sampleDepth >= SKY_DEPTH_THRESHOLD)
	{
		return;
	}
	float w = spatialWeight * depthWeight(centerDepth, sampleDepth, depthSigma);
	if (w <= 0.0)
	{
		return;
	}
	result += texture(occlusionTexture, sampleUV).r * w;
	weightSum += w;
}

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	float centerDepth = texture(depthTexture, uv).r;
	if (centerDepth >= SKY_DEPTH_THRESHOLD)
	{
		FragColor = vec4(1.0);
		return;
	}

	float result = texture(occlusionTexture, uv).r * WEIGHT0;
	float weightSum = WEIGHT0;

	int pairs = int(tapPairs + 0.5);
	if (pairs >= 1)
	{
		accumulateTap(result, weightSum, uv + blurDirection * 1.0, centerDepth, WEIGHT1);
		accumulateTap(result, weightSum, uv - blurDirection * 1.0, centerDepth, WEIGHT1);
	}
	if (pairs >= 2)
	{
		accumulateTap(result, weightSum, uv + blurDirection * 2.0, centerDepth, WEIGHT2);
		accumulateTap(result, weightSum, uv - blurDirection * 2.0, centerDepth, WEIGHT2);
	}
	if (pairs >= 3)
	{
		accumulateTap(result, weightSum, uv + blurDirection * 3.0, centerDepth, WEIGHT3);
		accumulateTap(result, weightSum, uv - blurDirection * 3.0, centerDepth, WEIGHT3);
	}
	if (pairs >= 4)
	{
		accumulateTap(result, weightSum, uv + blurDirection * 4.0, centerDepth, WEIGHT4);
		accumulateTap(result, weightSum, uv - blurDirection * 4.0, centerDepth, WEIGHT4);
	}

	result /= max(weightSum, 1e-6);
	FragColor = vec4(vec3(result), 1.0);
}
