// Shared bilateral-kernel policy for separable screen-space blurs.
// Effect wrappers retain value validity and accumulation semantics: SSAO treats
// invalid/sky as unoccluded scalar data; SSR treats them as no-confidence color.

const float WZ_BLUR_SKY_DEPTH_THRESHOLD = 0.9999;

float wzBlurGaussianWeight(int tap)
{
	if (tap == 0) return 0.227027;
	if (tap == 1) return 0.1945946;
	if (tap == 2) return 0.1216216;
	if (tap == 3) return 0.054054;
	return 0.016216;
}

float wzBlurDepthWeight(float centerDepth, float sampleDepth, float sigma)
{
	float d = centerDepth - sampleDepth;
	float s = max(sigma, 1e-6);
	return exp(-(d * d) / (2.0 * s * s));
}

bool wzBlurPairEnabled(float tapPairs, int pair)
{
	return int(tapPairs + 0.5) >= pair;
}
