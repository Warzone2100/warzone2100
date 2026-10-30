vec3 bloomTentTap(sampler2D tex, vec2 uv, vec2 lo, vec2 maxUv)
{
	return texture(tex, clamp(uv, lo, maxUv)).rgb;
}

// 3x3 tent around `uv` (`texel` = 1 / texture size, `radius` in texels)
vec3 bloomTent(sampler2D tex, vec2 uv, vec2 texel, float radius, vec2 maxUv)
{
	vec2 d = texel * radius;
	vec2 lo = 0.5 * texel;
	vec3 sum = 4.0 * bloomTentTap(tex, uv, lo, maxUv);
	sum += 2.0 * bloomTentTap(tex, uv + vec2(-d.x, 0.0), lo, maxUv);
	sum += 2.0 * bloomTentTap(tex, uv + vec2(d.x, 0.0), lo, maxUv);
	sum += 2.0 * bloomTentTap(tex, uv + vec2(0.0, -d.y), lo, maxUv);
	sum += 2.0 * bloomTentTap(tex, uv + vec2(0.0, d.y), lo, maxUv);
	sum += bloomTentTap(tex, uv + vec2(-d.x, -d.y), lo, maxUv);
	sum += bloomTentTap(tex, uv + vec2(d.x, -d.y), lo, maxUv);
	sum += bloomTentTap(tex, uv + vec2(-d.x, d.y), lo, maxUv);
	sum += bloomTentTap(tex, uv + vec2(d.x, d.y), lo, maxUv);
	return sum * (1.0 / 16.0);
}
