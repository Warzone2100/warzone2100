// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform vec2 blurDirection;
uniform float depthSigma;
uniform vec4 uvScaleClamp;
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
		#ifdef NEWGL
		FragColor = vec4(1.0);
		#else
		gl_FragColor = vec4(1.0);
		#endif
		return;
	}

	float result = texture(occlusionTexture, uv).r * WEIGHT0;
	float weightSum = WEIGHT0;

	accumulateTap(result, weightSum, uv + blurDirection * 1.0, centerDepth, WEIGHT1);
	accumulateTap(result, weightSum, uv - blurDirection * 1.0, centerDepth, WEIGHT1);
	accumulateTap(result, weightSum, uv + blurDirection * 2.0, centerDepth, WEIGHT2);
	accumulateTap(result, weightSum, uv - blurDirection * 2.0, centerDepth, WEIGHT2);
	accumulateTap(result, weightSum, uv + blurDirection * 3.0, centerDepth, WEIGHT3);
	accumulateTap(result, weightSum, uv - blurDirection * 3.0, centerDepth, WEIGHT3);
	accumulateTap(result, weightSum, uv + blurDirection * 4.0, centerDepth, WEIGHT4);
	accumulateTap(result, weightSum, uv - blurDirection * 4.0, centerDepth, WEIGHT4);

	result /= max(weightSum, 1e-6);

	#ifdef NEWGL
	FragColor = vec4(vec3(result), 1.0);
	#else
	gl_FragColor = vec4(vec3(result), 1.0);
	#endif
}
