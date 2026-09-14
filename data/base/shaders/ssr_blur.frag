// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	vec2 blurDirection;
	float depthSigma;
	float tapPairs;
	vec4 valueUvScaleClamp;
	vec4 depthUvScaleClamp;
};
uniform sampler2D ssrTexture;
uniform sampler2D depthTexture;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex, uv) texture2D(tex, uv)
#endif

#ifdef NEWGL
in vec2 texCoords;
out vec4 FragColor;
#else
varying vec2 texCoords;
#endif

#include "depth_aware_blur.glsl"

// Must match HIT_CONFIDENCE_MIN in ssr_generate.frag. Miss alpha is 0.3-0.65.
const float SSR_BLUR_HIT_ALPHA = 0.75;

void writeColor(vec4 color)
{
	#ifdef NEWGL
	FragColor = color;
	#else
	gl_FragColor = color;
	#endif
}

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
		writeColor(vec4(0.0));
		return;
	}

	vec2 ssrUv = clamp(texCoords * valueUvScaleClamp.xy, vec2(0.0), valueUvScaleClamp.zw);
	vec4 center = texture(ssrTexture, ssrUv);
	if (center.a < 1e-3)
	{
		writeColor(vec4(0.0));
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
	writeColor(vec4(color, confidence));
}
