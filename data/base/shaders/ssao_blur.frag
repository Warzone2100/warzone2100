// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	vec2 blurDirection;
	float depthSigma;
	float tapPairs;
	vec4 occlusionUvScaleClamp;
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

void accumulateTap(inout float result, inout float weightSum, vec2 tapTexCoords, float centerDepth, float spatialWeight)
{
	vec2 sampleDepthUV = clamp(tapTexCoords * depthUvScaleClamp.xy, vec2(0.0), depthUvScaleClamp.zw);
	float sampleDepth = texture(depthTexture, sampleDepthUV).r;
	if (sampleDepth >= SKY_DEPTH_THRESHOLD)
	{
		return;
	}
	float w = spatialWeight * depthWeight(centerDepth, sampleDepth, depthSigma);
	if (w <= 0.0)
	{
		return;
	}
	vec2 sampleAoUV = clamp(tapTexCoords * occlusionUvScaleClamp.xy, vec2(0.0), occlusionUvScaleClamp.zw);
	result += texture(occlusionTexture, sampleAoUV).r * w;
	weightSum += w;
}

void main()
{
	vec2 depthUv = clamp(texCoords * depthUvScaleClamp.xy, vec2(0.0), depthUvScaleClamp.zw);
	float centerDepth = texture(depthTexture, depthUv).r;
	if (centerDepth >= SKY_DEPTH_THRESHOLD)
	{
		#ifdef NEWGL
		FragColor = vec4(1.0);
		#else
		gl_FragColor = vec4(1.0);
		#endif
		return;
	}

	vec2 aoUv = clamp(texCoords * occlusionUvScaleClamp.xy, vec2(0.0), occlusionUvScaleClamp.zw);
	float result = texture(occlusionTexture, aoUv).r * WEIGHT0;
	float weightSum = WEIGHT0;

	int pairs = int(tapPairs + 0.5);
	if (pairs >= 1)
	{
		accumulateTap(result, weightSum, texCoords + blurDirection * 1.0, centerDepth, WEIGHT1);
		accumulateTap(result, weightSum, texCoords - blurDirection * 1.0, centerDepth, WEIGHT1);
	}
	if (pairs >= 2)
	{
		accumulateTap(result, weightSum, texCoords + blurDirection * 2.0, centerDepth, WEIGHT2);
		accumulateTap(result, weightSum, texCoords - blurDirection * 2.0, centerDepth, WEIGHT2);
	}
	if (pairs >= 3)
	{
		accumulateTap(result, weightSum, texCoords + blurDirection * 3.0, centerDepth, WEIGHT3);
		accumulateTap(result, weightSum, texCoords - blurDirection * 3.0, centerDepth, WEIGHT3);
	}
	if (pairs >= 4)
	{
		accumulateTap(result, weightSum, texCoords + blurDirection * 4.0, centerDepth, WEIGHT4);
		accumulateTap(result, weightSum, texCoords - blurDirection * 4.0, centerDepth, WEIGHT4);
	}

	result /= max(weightSum, 1e-6);

	#ifdef NEWGL
	FragColor = vec4(vec3(result), 1.0);
	#else
	gl_FragColor = vec4(vec3(result), 1.0);
	#endif
}
