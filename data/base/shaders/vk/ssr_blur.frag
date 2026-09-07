#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec2 blurDirection;
	float depthSigma;
	float tapPairs;
	vec4 valueUvScaleClamp;
	vec4 depthUvScaleClamp;
};

layout(set = 1, binding = 0) uniform sampler2D ssrTexture;
layout(set = 1, binding = 1) uniform sampler2D depthTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

#include "depth_aware_blur.glsl"

// Must match HIT_CONFIDENCE_MIN in ssr_generate.frag. Miss alpha is 0.3-0.65.
const float SSR_BLUR_HIT_ALPHA = 0.75;

void accumulateTap(inout vec3 color, inout float colorWeight, inout float confidence, inout float confWeight,
	vec2 tapTexCoords, float centerDepth, float centerConfidence, float spatialWeight)
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
	vec2 sampleSsrUV = clamp(tapTexCoords * valueUvScaleClamp.xy, vec2(0.0), valueUvScaleClamp.zw);
	vec4 sampleSsr = texture(ssrTexture, sampleSsrUV);
	if (sampleSsr.a < 1e-3)
	{
		return;
	}
	// Reflector depth is flat on a lake; do not mix sky misses into unit hits.
	bool centerHit = centerConfidence >= SSR_BLUR_HIT_ALPHA;
	bool sampleHit = sampleSsr.a >= SSR_BLUR_HIT_ALPHA;
	if (centerHit != sampleHit)
	{
		return;
	}
	color += sampleSsr.rgb * w;
	colorWeight += w;
	confidence += sampleSsr.a * w;
	confWeight += w;
}

void main()
{
	vec2 depthUv = clamp(texCoords * depthUvScaleClamp.xy, vec2(0.0), depthUvScaleClamp.zw);
	float centerDepth = texture(depthTexture, depthUv).r;
	if (centerDepth >= WZ_BLUR_SKY_DEPTH_THRESHOLD)
	{
		FragColor = vec4(0.0);
		return;
	}

	vec2 ssrUv = clamp(texCoords * valueUvScaleClamp.xy, vec2(0.0), valueUvScaleClamp.zw);
	vec4 center = texture(ssrTexture, ssrUv);
	if (center.a < 1e-3)
	{
		FragColor = vec4(0.0);
		return;
	}

	vec3 color = center.rgb * wzBlurGaussianWeight(0);
	float colorWeight = wzBlurGaussianWeight(0);
	float confidence = center.a * wzBlurGaussianWeight(0);
	float confWeight = wzBlurGaussianWeight(0);

	if (wzBlurPairEnabled(tapPairs, 1))
	{
		accumulateTap(color, colorWeight, confidence, confWeight, texCoords + blurDirection * 1.0, centerDepth, center.a, wzBlurGaussianWeight(1));
		accumulateTap(color, colorWeight, confidence, confWeight, texCoords - blurDirection * 1.0, centerDepth, center.a, wzBlurGaussianWeight(1));
	}
	if (wzBlurPairEnabled(tapPairs, 2))
	{
		accumulateTap(color, colorWeight, confidence, confWeight, texCoords + blurDirection * 2.0, centerDepth, center.a, wzBlurGaussianWeight(2));
		accumulateTap(color, colorWeight, confidence, confWeight, texCoords - blurDirection * 2.0, centerDepth, center.a, wzBlurGaussianWeight(2));
	}
	if (wzBlurPairEnabled(tapPairs, 3))
	{
		accumulateTap(color, colorWeight, confidence, confWeight, texCoords + blurDirection * 3.0, centerDepth, center.a, wzBlurGaussianWeight(3));
		accumulateTap(color, colorWeight, confidence, confWeight, texCoords - blurDirection * 3.0, centerDepth, center.a, wzBlurGaussianWeight(3));
	}
	if (wzBlurPairEnabled(tapPairs, 4))
	{
		accumulateTap(color, colorWeight, confidence, confWeight, texCoords + blurDirection * 4.0, centerDepth, center.a, wzBlurGaussianWeight(4));
		accumulateTap(color, colorWeight, confidence, confWeight, texCoords - blurDirection * 4.0, centerDepth, center.a, wzBlurGaussianWeight(4));
	}

	color /= max(colorWeight, 1e-6);
	confidence /= max(confWeight, 1e-6);
	FragColor = vec4(color, confidence);
}
