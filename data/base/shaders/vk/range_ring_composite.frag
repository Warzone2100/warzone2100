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

float channelOverlay(float field)
{
	float w = max(fwidth(field), 1e-4);
	float uncovered = step(0.999, field);
	float covered = 1.0 - uncovered;
	float ring = (1.0 - smoothstep(0.0, w, abs(field - 0.5))) * covered;
	float fill = (1.0 - smoothstep(0.5, 0.5 + w, field)) * fillAlpha * covered;
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
			float sensor = channelOverlay(field.r);
			float weapon = channelOverlay(field.g);
			float minRange = channelOverlay(field.b);
			vec3 overlay = sensorColor.rgb * sensor + weaponColor.rgb * weapon + minRangeColor.rgb * minRange;
			float a = clamp(max(max(sensor, weapon), minRange), 0.0, 1.0);
			result = lit * (1.0 - a) + overlay;
		}
	}

	FragColor = vec4(result, 1.0);
}
