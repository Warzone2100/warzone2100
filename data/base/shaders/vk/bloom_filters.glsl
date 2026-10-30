// Bloom filters, adapted from Jimenez (SIGGRAPH 2014).
// Expects `sourceTexture` and its `uvScaleClamp`
// (xy = used extent / texture size, zw = largest UV inside the used extent).

#include "bloom_tent.glsl"

vec3 bloomTap(vec2 uv, vec2 texel)
{
	return texture(sourceTexture, clamp(uv, 0.5 * texel, uvScaleClamp.zw)).rgb;
}

float bloomLuma(vec3 c)
{
	return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// Keeps the light above `threshold`, with a quadratic soft knee over threshold - knee .. threshold + knee
vec3 bloomThreshold(vec3 c, float threshold, float knee)
{
	float brightness = max(c.r, max(c.g, c.b));
	float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
	soft = soft * soft / (4.0 * knee + 1e-5);
	float contribution = max(soft, brightness - threshold) / max(brightness, 1e-5);
	return c * contribution;
}

// A scene tap, with NaN and infinity dropped, then thresholded.
// NOTE: Thresholding each tap before any averaging keeps thin bright lines (ex. laser bolts) blooming.
vec3 bloomSceneTap(vec2 uv, vec2 texel, float threshold)
{
	vec3 c = bloomTap(uv, texel);
	c = mix(c, vec3(0.0), vec3(isnan(c)));
	c = clamp(c, vec3(0.0), vec3(65000.0));
	return bloomThreshold(c, threshold, 0.25 * threshold);
}

float bloomKarisWeight(vec3 box)
{
	return 1.0 / (1.0 + bloomLuma(box));
}

vec3 bloomDownsampleTap(vec2 uv, vec2 texel, float threshold)
{
	return (threshold > 0.0) ? bloomSceneTap(uv, texel, threshold) : bloomTap(uv, texel);
}

// 13 taps in five overlapping 2x2 boxes, to half the source size (`texel` = 1 / source size).
// With a threshold (the first step, reading the scene), the taps are thresholded and the boxes Karis averaged.
vec3 bloomDownsample13(vec2 uv, vec2 texel, float threshold)
{
	vec3 a = bloomDownsampleTap(uv + texel * vec2(-2.0, -2.0), texel, threshold);
	vec3 b = bloomDownsampleTap(uv + texel * vec2( 0.0, -2.0), texel, threshold);
	vec3 c = bloomDownsampleTap(uv + texel * vec2( 2.0, -2.0), texel, threshold);
	vec3 d = bloomDownsampleTap(uv + texel * vec2(-1.0, -1.0), texel, threshold);
	vec3 e = bloomDownsampleTap(uv + texel * vec2( 1.0, -1.0), texel, threshold);
	vec3 f = bloomDownsampleTap(uv + texel * vec2(-2.0,  0.0), texel, threshold);
	vec3 g = bloomDownsampleTap(uv, texel, threshold);
	vec3 h = bloomDownsampleTap(uv + texel * vec2( 2.0,  0.0), texel, threshold);
	vec3 i = bloomDownsampleTap(uv + texel * vec2(-1.0,  1.0), texel, threshold);
	vec3 j = bloomDownsampleTap(uv + texel * vec2( 1.0,  1.0), texel, threshold);
	vec3 k = bloomDownsampleTap(uv + texel * vec2(-2.0,  2.0), texel, threshold);
	vec3 l = bloomDownsampleTap(uv + texel * vec2( 0.0,  2.0), texel, threshold);
	vec3 m = bloomDownsampleTap(uv + texel * vec2( 2.0,  2.0), texel, threshold);

	vec3 center = (d + e + i + j) * 0.25;
	vec3 topLeft = (a + b + f + g) * 0.25;
	vec3 topRight = (b + c + g + h) * 0.25;
	vec3 bottomLeft = (f + g + k + l) * 0.25;
	vec3 bottomRight = (g + h + l + m) * 0.25;

	if (threshold <= 0.0)
	{
		return center * 0.5 + (topLeft + topRight + bottomLeft + bottomRight) * 0.125;
	}
	float wc = 0.5 * bloomKarisWeight(center);
	float w0 = 0.125 * bloomKarisWeight(topLeft);
	float w1 = 0.125 * bloomKarisWeight(topRight);
	float w2 = 0.125 * bloomKarisWeight(bottomLeft);
	float w3 = 0.125 * bloomKarisWeight(bottomRight);
	return (center * wc + topLeft * w0 + topRight * w1 + bottomLeft * w2 + bottomRight * w3)
		/ max(wc + w0 + w1 + w2 + w3, 1e-4);
}

// 9 taps fitted to the 13-tap filter's weights, after Gjoel and Svendsen (GDC 2016).
// Only valid when the destination is exactly half the source size.
vec3 bloomDownsample9(vec2 uv, vec2 texel)
{
	const float EDGE = 1.7940618;
	const float CORNER = 1.75;
	vec3 center = bloomTap(uv, texel);
	vec3 edges = bloomTap(uv + texel * vec2(-EDGE, 0.0), texel)
		+ bloomTap(uv + texel * vec2(EDGE, 0.0), texel)
		+ bloomTap(uv + texel * vec2(0.0, -EDGE), texel)
		+ bloomTap(uv + texel * vec2(0.0, EDGE), texel);
	vec3 corners = bloomTap(uv + texel * vec2(-CORNER, -CORNER), texel)
		+ bloomTap(uv + texel * vec2(CORNER, -CORNER), texel)
		+ bloomTap(uv + texel * vec2(-CORNER, CORNER), texel)
		+ bloomTap(uv + texel * vec2(CORNER, CORNER), texel);
	return center * 0.24634435 + edges * 0.12575353 + corners * 0.06266039;
}

vec3 bloomUpsampleTent(vec2 uv, vec2 texel, float radius)
{
	return bloomTent(sourceTexture, uv, texel, radius, uvScaleClamp.zw);
}
