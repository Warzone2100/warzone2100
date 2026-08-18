#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 orthoViewProj;
	vec4 mapOriginExtent; // xy origin.xz, zw size.xz
	vec4 sdfParams; // x = sdfBand
};

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec3 centerRadius;

layout(location = 0) out vec4 FragColor;

// SDF of AABB [origin, origin+extent], negative inside.
float sdBox(vec2 p, vec2 origin, vec2 extent)
{
	vec2 halfExt = 0.5 * extent;
	vec2 q = abs(p - (origin + halfExt)) - halfExt; // >0 outside on that axis
	return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0);
}

void main()
{
	float R = max(centerRadius.z, 1e-3);
	// Analytic 2D disk SDF (not distance-to-cone). Negative inside, 0 on the ring.
	float diskSdf = length(worldPos.xz - centerRadius.xy) - R;
	float sdf = diskSdf;
	if (mapOriginExtent.z > 0.0 && mapOriginExtent.w > 0.0)
	{
		// Intersection of the disk and the map, so the isocontour follows the map edge when range is clipped.
		sdf = max(diskSdf, sdBox(worldPos.xz, mapOriginExtent.xy, mapOriginExtent.zw));
	}
	// Outside the encoded band: keep the clear (1 = uncovered). Cone raster already
	// limited coverage to the 1.25R disk; this clips the unused outer skirt.
	if (sdf > sdfParams.x)
	{
		discard;
	}
	float band = max(sdfParams.x, 1e-3);
	// Map [-band, +band] -> [0, 1]; 0.5 is the isocontour, 1 is uncovered.
	float encoded = 0.5 + 0.5 * clamp(sdf / band, -1.0, 1.0);
	FragColor = vec4(encoded, encoded, encoded, 1.0);
}
