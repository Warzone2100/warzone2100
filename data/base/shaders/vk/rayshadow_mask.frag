#version 460
#extension GL_EXT_ray_query : require

// Ray-traced sun-shadow + ambient-occlusion mask pass.
// Casts one cone-jittered ray toward the sun (soft shadows) plus a few short cosine-hemisphere
// rays (contact shadows / AO) per pixel, using the camera depth prepass to reconstruct world
// positions. Output: R = sun visibility (1 = lit), G = ambient occlusion (1 = open sky).

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 1) uniform sampler2D depthTexture;

layout(push_constant) uniform PushConstants
{
	mat4 invProjectionView; // inverse of the GL-convention projection * view matrix
	vec4 sunDirection;      // xyz = normalized direction toward the sun (world space)
	vec4 cameraPosition;    // xyz = camera position (world space)
	vec4 params;            // x = ray tMin, y = ray tMax, z = normal offset scale, w = sun cone angle (radians)
	vec4 aoParams;          // x = AO ray count, y = AO max distance, z = AO strength, w = unused
};

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec2 visibility;

// NOTE on orientation: the engine's depth-only vertex shaders intentionally do NOT apply the
// Vulkan y-flip (the cascade shadow-map sampling convention depends on that), so the camera
// depth prepass image is vertically mirrored relative to screen orientation. Flip v here.
float sampleSceneDepth(vec2 uv)
{
	return texture(depthTexture, vec2(uv.x, 1.0 - uv.y)).r;
}

vec3 reconstructWorldPos(vec2 uv, float depth)
{
	// Undo the engine's clip-space z remap (gl_Position.z = (z + w) / 2), then unproject with
	// the GL-convention inverse projection-view matrix. The depth prepass rasterizes WITHOUT
	// the y-flip, so after the v-flip in sampleSceneDepth our uv is already screen-oriented:
	// screen v = (1 - gl_ndc.y) / 2  =>  gl_ndc.y = 1 - 2 * v.
	vec4 clip = vec4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, 2.0 * depth - 1.0, 1.0);
	vec4 world = invProjectionView * clip;
	return world.xyz / world.w;
}

// Interleaved gradient noise - stable per-pixel randomization (no temporal accumulation)
float interleavedGradientNoise(vec2 pixel)
{
	return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

mat3 buildBasis(vec3 n)
{
	vec3 up = (abs(n.y) < 0.99) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 t = normalize(cross(up, n));
	vec3 b = cross(n, t);
	return mat3(t, b, n);
}

bool traceOcclusion(vec3 origin, vec3 dir, float tMin, float tMax)
{
	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, topLevelAS,
						  gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
						  0xFF, origin, tMin, dir, tMax);
	rayQueryProceedEXT(rayQuery);
	return rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT;
}

void main()
{
	float depth = sampleSceneDepth(texCoords);
	if (depth >= 1.0)
	{
		visibility = vec2(1.0, 1.0); // sky / no geometry
		return;
	}

	vec3 worldPos = reconstructWorldPos(texCoords, depth);

	// Reconstruct the surface normal from the depth buffer using one-sided differences,
	// picking per axis whichever neighbor has the smaller depth delta. Unlike dFdx/dFdy
	// (which straddle face boundaries and silhouettes, producing invalid normals on
	// small or detailed geometry), this stays on the same surface at edges.
	vec2 texel = 1.0 / vec2(textureSize(depthTexture, 0));
	float dL = sampleSceneDepth(texCoords - vec2(texel.x, 0.0));
	float dR = sampleSceneDepth(texCoords + vec2(texel.x, 0.0));
	float dU = sampleSceneDepth(texCoords - vec2(0.0, texel.y));
	float dD = sampleSceneDepth(texCoords + vec2(0.0, texel.y));
	vec3 ddx = (abs(dR - depth) <= abs(dL - depth))
		? (reconstructWorldPos(texCoords + vec2(texel.x, 0.0), dR) - worldPos)
		: (worldPos - reconstructWorldPos(texCoords - vec2(texel.x, 0.0), dL));
	vec3 ddy = (abs(dD - depth) <= abs(dU - depth))
		? (reconstructWorldPos(texCoords + vec2(0.0, texel.y), dD) - worldPos)
		: (worldPos - reconstructWorldPos(texCoords - vec2(0.0, texel.y), dU));
	vec3 geomNormal = cross(ddx, ddy);
	float normalLen = length(geomNormal);
	geomNormal = (normalLen > 0.0001) ? (geomNormal / normalLen) : vec3(0.0, 1.0, 0.0);
	// enforce camera-facing orientation (the cross product's sign depends on winding)
	if (dot(geomNormal, cameraPosition.xyz - worldPos) < 0.0)
	{
		geomNormal = -geomNormal;
	}

	float noise = interleavedGradientNoise(gl_FragCoord.xy);

	// --- sun shadow: one ray, jittered within the sun's angular cone for penumbra ---
	// Always trace (no backface early-out): the reconstructed face normal is noisier than
	// the smooth shading normals used by the lighting shaders, and a hard zero at its
	// terminator would bleed through the blur onto lit neighbors. Genuinely self-shadowed
	// surfaces block their own ray and return 0 at the geometrically correct boundary.
	float sunNdotL = dot(geomNormal, sunDirection.xyz);
	// slope-scaled origin offset (the ray-traced equivalent of shadow-map slope bias),
	// pushed out on the sun-facing side of the surface
	vec3 sunSideNormal = (sunNdotL < 0.0) ? -geomNormal : geomNormal;
	float slopeScale = clamp(1.0 / max(abs(sunNdotL), 0.2), 1.0, 5.0);
	vec3 origin = worldPos + sunSideNormal * (params.z * slopeScale);

	float coneAngle = params.w;
	vec3 sunDir = sunDirection.xyz;
	if (coneAngle > 0.0)
	{
		mat3 sunBasis = buildBasis(sunDir);
		float r = sqrt(noise) * tan(coneAngle);
		float phi = 6.2831853 * interleavedGradientNoise(gl_FragCoord.yx + vec2(17.0, 59.0));
		sunDir = normalize(sunBasis * vec3(r * cos(phi), r * sin(phi), 1.0));
	}

	visibility.r = traceOcclusion(origin, sunDir, params.x, params.y) ? 0.0 : 1.0;

	// --- ambient occlusion / contact shadows: a fixed, deterministic cone of rays around the
	// surface normal, anchored to a world-space frame. No stochastic sampling: random hemisphere
	// rays are highly divergent (slow on some GPUs) and produce screen-anchored noise that
	// "swims" when the camera scrolls. A deterministic pattern is noise-free and coherent.
	int aoRayCount = int(aoParams.x);
	if (aoRayCount > 0)
	{
		// world-space tangent frame around the normal (stable under camera movement)
		vec3 tangentSeed = (abs(geomNormal.y) < 0.99) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
		vec3 tangent = normalize(cross(tangentSeed, geomNormal));
		vec3 bitangent = cross(geomNormal, tangent);

		vec3 aoOrigin = worldPos + geomNormal * (params.z * 2.0);
		const float CONE_TILT = 0.7; // ~45 degrees off the normal - catches walls and overhangs
		float occlusion = 0.0;
		float totalWeight = 0.0;
		for (int i = 0; i < aoRayCount; ++i)
		{
			vec3 dir;
			if (i < 3)
			{
				// three rays tilted around the normal at fixed 120-degree azimuths
				float phi = 2.0943951 * float(i); // 2*pi/3
				dir = normalize(geomNormal + (tangent * cos(phi) + bitangent * sin(phi)) * CONE_TILT);
			}
			else
			{
				dir = geomNormal; // optional 4th ray straight along the normal
			}
			rayQueryEXT aoQuery;
			rayQueryInitializeEXT(aoQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF,
								  aoOrigin, params.x * 2.0, dir, aoParams.y);
			rayQueryProceedEXT(aoQuery);
			if (rayQueryGetIntersectionTypeEXT(aoQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
			{
				// distance-weighted: contact-range hits occlude fully, farther hits fade out
				float hitT = rayQueryGetIntersectionTEXT(aoQuery, true);
				occlusion += 1.0 - clamp(hitT / aoParams.y, 0.0, 1.0);
			}
			totalWeight += 1.0;
		}
		// fade AO out on vertical surfaces: contact shadows belong primarily on up-facing
		// surfaces (walls otherwise accumulate occlusion from the ground plane)
		float upFacing = clamp(geomNormal.y, 0.0, 1.0);
		float aoStrength = aoParams.z * mix(0.25, 1.0, upFacing);
		float ao = 1.0 - (occlusion / max(totalWeight, 1.0)) * aoStrength;
		visibility.g = clamp(ao, 0.0, 1.0);
	}
	else
	{
		visibility.g = 1.0;
	}
}
