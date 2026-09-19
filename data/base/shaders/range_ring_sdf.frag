// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

layout(std140) uniform cbuffer {
	mat4 orthoViewProj;
	vec4 mapOriginExtent; // xy origin.xz, zw size.xz
	vec4 sdfParams; // x = sdfBand
};

#ifdef NEWGL
in vec3 worldPos;
in vec3 centerRadius;
out vec4 FragColor;
#else
varying vec3 worldPos;
varying vec3 centerRadius;
#endif

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
	#ifdef NEWGL
	FragColor = vec4(encoded, encoded, encoded, 1.0);
	#else
	gl_FragColor = vec4(encoded, encoded, encoded, 1.0);
	#endif
}
