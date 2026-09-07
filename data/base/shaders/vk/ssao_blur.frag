#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec2 blurDirection;
	float depthSigma;
	float tapPairs;
	vec4 valueUvScaleClamp;
	vec4 depthUvScaleClamp;
};

layout(set = 1, binding = 0) uniform sampler2D occlusionTexture;
layout(set = 1, binding = 1) uniform sampler2D depthTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

#include "depth_aware_blur.glsl"

void accumulateTap(inout float result, inout float weightSum, vec2 tapTexCoords, float centerDepth, float spatialWeight)
{
	vec2 sampleDepthUV = clamp(tapTexCoords * depthUvScaleClamp.xy, vec2(0.0), depthUvScaleClamp.zw);
	float sampleDepth = texture(depthTexture, sampleDepthUV).r;
	if (sampleDepth >= WZ_BLUR_SKY_DEPTH_THRESHOLD)
	{
		return;
	}
	float w = spatialWeight * wzBlurDepthWeight(centerDepth, sampleDepth, depthSigma);
	if (w <= 0.0)
	{
		return;
	}
	vec2 sampleAoUV = clamp(tapTexCoords * valueUvScaleClamp.xy, vec2(0.0), valueUvScaleClamp.zw);
	result += texture(occlusionTexture, sampleAoUV).r * w;
	weightSum += w;
}

void main()
{
	vec2 depthUv = clamp(texCoords * depthUvScaleClamp.xy, vec2(0.0), depthUvScaleClamp.zw);
	float centerDepth = texture(depthTexture, depthUv).r;
	if (centerDepth >= WZ_BLUR_SKY_DEPTH_THRESHOLD)
	{
		FragColor = vec4(1.0);
		return;
	}

	vec2 aoUv = clamp(texCoords * valueUvScaleClamp.xy, vec2(0.0), valueUvScaleClamp.zw);
	float result = texture(occlusionTexture, aoUv).r * wzBlurGaussianWeight(0);
	float weightSum = wzBlurGaussianWeight(0);

	if (wzBlurPairEnabled(tapPairs, 1))
	{
		accumulateTap(result, weightSum, texCoords + blurDirection * 1.0, centerDepth, wzBlurGaussianWeight(1));
		accumulateTap(result, weightSum, texCoords - blurDirection * 1.0, centerDepth, wzBlurGaussianWeight(1));
	}
	if (wzBlurPairEnabled(tapPairs, 2))
	{
		accumulateTap(result, weightSum, texCoords + blurDirection * 2.0, centerDepth, wzBlurGaussianWeight(2));
		accumulateTap(result, weightSum, texCoords - blurDirection * 2.0, centerDepth, wzBlurGaussianWeight(2));
	}
	if (wzBlurPairEnabled(tapPairs, 3))
	{
		accumulateTap(result, weightSum, texCoords + blurDirection * 3.0, centerDepth, wzBlurGaussianWeight(3));
		accumulateTap(result, weightSum, texCoords - blurDirection * 3.0, centerDepth, wzBlurGaussianWeight(3));
	}
	if (wzBlurPairEnabled(tapPairs, 4))
	{
		accumulateTap(result, weightSum, texCoords + blurDirection * 4.0, centerDepth, wzBlurGaussianWeight(4));
		accumulateTap(result, weightSum, texCoords - blurDirection * 4.0, centerDepth, wzBlurGaussianWeight(4));
	}

	result /= max(weightSum, 1e-6);
	FragColor = vec4(vec3(result), 1.0);
}
