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
	float padding0;
	float padding1;
	float padding2;
};

layout(set = 1, binding = 0) uniform sampler2D sceneTexture;
layout(set = 1, binding = 1) uniform sampler2D prepassDepth;
layout(set = 1, binding = 2) uniform sampler2D rangeRingSdf;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

const float SKY_DEPTH_THRESHOLD = 0.9999;
// Max AA width in sdf/R. Uncapped fwidth(sd) explodes at depth silhouettes
// (cliffs, bump edges) because reconstructed world XZ jumps between neighbors.
const float SDF_AA_MAX = 0.04;
const float SDF_AA_SILHOUETTE = 0.008;

float overlayAaLimit(vec2 worldXZ)
{
	float spanX = length(dFdx(worldXZ));
	float spanY = length(dFdy(worldXZ));
	float spanMax = max(spanX, spanY);
	float spanMin = min(spanX, spanY) + 1e-3;
	return (spanMax > 4.0 * spanMin) ? SDF_AA_SILHOUETTE : SDF_AA_MAX;
}

float channelOverlay(float field, float maxW)
{
	// encoded = 0.5 + 0.5 * clamp(sdf/R, -1, 1); cone stores at most sdf/R = 0.25.
	float sd = field * 2.0 - 1.0;
	float uncovered = step(0.999, field);
	float covered = 1.0 - uncovered;
	float w = min(max(fwidth(min(sd, 0.25)), 1e-4), maxW);
	float ring = (1.0 - smoothstep(0.0, w, abs(sd))) * covered;
	float fill = (1.0 - smoothstep(0.0, w, max(sd, 0.0))) * fillAlpha * covered;
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
		vec2 sdfUv = (worldPos.xz - sdfOriginExtent.xy) / max(sdfOriginExtent.zw, vec2(1e-3));
		if (all(greaterThanEqual(sdfUv, vec2(0.0))) && all(lessThanEqual(sdfUv, vec2(1.0))))
		{
			sdfUv = clamp(sdfUv * sdfUvScaleClamp.xy, vec2(0.0), sdfUvScaleClamp.zw);
			vec3 field = texture(rangeRingSdf, sdfUv).rgb;
			float maxW = overlayAaLimit(worldPos.xz);
			float sensor = channelOverlay(field.r, maxW);
			float weapon = channelOverlay(field.g, maxW);
			float minRange = channelOverlay(field.b, maxW);
			vec3 overlay = sensorColor.rgb * sensor + weaponColor.rgb * weapon + minRangeColor.rgb * minRange;
			float a = clamp(max(max(sensor, weapon), minRange), 0.0, 1.0);
			result = lit * (1.0 - a) + overlay;
		}
	}

	FragColor = vec4(result, 1.0);
}
