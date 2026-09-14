// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	vec2 blurDirection;
	float depthSigma;
	float tapPairs;
	vec4 valueUvScaleClamp;
	vec4 depthUvScaleClamp;
};
uniform sampler2D occlusionTexture;
uniform sampler2D depthTexture;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex, uv) texture2D(tex, uv)
#endif

#ifdef NEWGL
in vec2 texCoords;
#else
varying vec2 texCoords;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

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
		#ifdef NEWGL
		FragColor = vec4(1.0);
		#else
		gl_FragColor = vec4(1.0);
		#endif
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

	#ifdef NEWGL
	FragColor = vec4(vec3(result), 1.0);
	#else
	gl_FragColor = vec4(vec3(result), 1.0);
	#endif
}
