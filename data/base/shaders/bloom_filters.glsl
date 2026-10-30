// Bloom filters, adapted from Jimenez (SIGGRAPH 2014).
// Expects `sourceTexture` and its `uvScaleClamp`
// (xy = used extent / texture size, zw = largest UV inside the used extent).

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

// 3x3 tent around `uv` (`radius` in source texels)
vec3 bloomUpsampleTent(vec2 uv, vec2 texel, float radius)
{
	vec2 d = texel * radius;
	vec3 sum = bloomTap(uv, texel) * 4.0;
	sum += (bloomTap(uv + vec2(-d.x, 0.0), texel) + bloomTap(uv + vec2(d.x, 0.0), texel)
		+ bloomTap(uv + vec2(0.0, -d.y), texel) + bloomTap(uv + vec2(0.0, d.y), texel)) * 2.0;
	sum += bloomTap(uv + vec2(-d.x, -d.y), texel) + bloomTap(uv + vec2(d.x, -d.y), texel)
		+ bloomTap(uv + vec2(-d.x, d.y), texel) + bloomTap(uv + vec2(d.x, d.y), texel);
	return sum * (1.0 / 16.0);
}
