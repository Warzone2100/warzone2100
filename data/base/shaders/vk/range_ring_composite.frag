#version 450

#include "view_position.glsl"

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 invProjectionMatrix;
	mat4 invViewMatrix;
	vec4 uvScaleClamp;
	vec4 sdfOriginExtent; // xy origin.xz, zw extent.xz (render world)
	vec4 sdfUvScaleClamp;
	vec4 sensorColor;
	vec4 weaponColor;
	vec4 minRangeColor;
	float fillAlpha;
	float sdfBand;
	float padding1;
	float padding2;
};

layout(set = 1, binding = 0) uniform sampler2D sceneTexture;
layout(set = 1, binding = 1) uniform sampler2D prepassDepth;
layout(set = 1, binding = 2) uniform sampler2D rangeRingSdf;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

const float SKY_DEPTH_THRESHOLD = 0.9999;
const float RING_HALF_PX = 2.0; // half-width of the smoothed ring, in pixels

// Cap pixel-width AA when world-XZ derivatives are highly anisotropic
// (silhouettes, depth jumps, map skirts) so fwidth does not paint a huge halo.
float overlayAaLimit(vec2 worldXZ, float band)
{
	float spanX = length(dFdx(worldXZ));
	float spanY = length(dFdy(worldXZ));
	float spanMax = max(spanX, spanY);
	float spanMin = min(spanX, spanY) + 1e-3;
	return (spanMax > 4.0 * spanMin) ? (band * 0.2) : band;
}

// Pixel-width smoothing of the SDF isocontour (sdf = 0).
float channelOverlay(float field, float band, float maxW)
{
	float sd = (field * 2.0 - 1.0) * band; // decode [0, 1] -> [-band, +band]
	float uncovered = step(0.999, field);  // generate-pass clear = outside all rings
	float covered = 1.0 - uncovered;
	// fwidth(sd) ~ how much signed distance changes across this pixel.
	float w = min(max(fwidth(min(sd, band)), 1e-4), maxW);
	float ring = (1.0 - smoothstep(0.0, RING_HALF_PX, abs(sd) / w)) * covered;
	float fill = (1.0 - smoothstep(0.0, RING_HALF_PX, max(sd, 0.0) / w)) * fillAlpha * covered;
	return max(ring, fill);
}

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	vec3 lit = texture(sceneTexture, uv).rgb;
	vec3 result = lit;
	float depth = texture(prepassDepth, uv).r;
	if (depth < SKY_DEPTH_THRESHOLD)
	{
		vec3 viewPos = wzGetViewPosition(uv, depth, invProjectionMatrix);
		vec3 worldPos = (invViewMatrix * vec4(viewPos, 1.0)).xyz;
		// Map terrain XZ into the frustum-fitted SDF atlas (same space as generate).
		vec2 sdfUv = (worldPos.xz - sdfOriginExtent.xy) / max(sdfOriginExtent.zw, vec2(1e-3));
		if (all(greaterThanEqual(sdfUv, vec2(0.0))) && all(lessThanEqual(sdfUv, vec2(1.0))))
		{
			sdfUv = clamp(sdfUv * sdfUvScaleClamp.xy, vec2(0.0), sdfUvScaleClamp.zw);
			vec3 field = texture(rangeRingSdf, sdfUv).rgb;
			float band = max(sdfBand, 1e-3);
			float maxW = overlayAaLimit(worldPos.xz, band);
			float sensor = channelOverlay(field.r, band, maxW);
			float weapon = channelOverlay(field.g, band, maxW);
			float minRange = channelOverlay(field.b, band, maxW);
			vec3 overlay = sensorColor.rgb * sensor + weaponColor.rgb * weapon + minRangeColor.rgb * minRange;
			float a = clamp(max(max(sensor, weapon), minRange), 0.0, 1.0);
			result = lit * (1.0 - a) + overlay; // premultiplied-style overlay
		}
	}

	FragColor = vec4(result, 1.0);
}
