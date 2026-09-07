#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 invProjectionMatrix;
	mat4 projectionMatrix;
	mat4 viewToSkyLocal;
	vec4 params;              // x=maxRayLength, y=thickness cap; z unused (start is MIN_RAY_START_ABS)
	vec4 prepassUvScaleClamp; // xy scale, zw clamp
	vec4 sceneUvScaleClamp;
	vec4 skyFogColor;         // rgb, a=fog enabled
	float stepCount;          // uploaded from quality presets; this shader does not read it
	float skyboxAvailable;
	float padding1;
	float padding2;
};

layout(set = 1, binding = 0) uniform sampler2D depthTexture;
layout(set = 1, binding = 1) uniform sampler2D normalsTexture;
layout(set = 1, binding = 2) uniform sampler2D sceneTexture;
layout(set = 1, binding = 3) uniform sampler2D skyboxTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

#include "view_position.glsl"
#include "sky_radiance.glsl"

// Water SSR: march reflect(V, N) with homogeneous (1/w) steps. A hit is the ray's
// view-Z interval overlapping a finite depth voxel. Toward-camera bounces that
// do not travel on screen miss to the skybox.
//
// McGuire & Mara, "Efficient GPU Screen-Space Ray Tracing", JCGT 3(4), 2014
//   https://jcgt.org/published/0003/04/04/
//   k = 1/w, rayView = Q/k (eq. 1); voxel overlap; near-plane clip; step budget.
// van Dongen, "Screen Space Reflections in Blightbound"
//   http://joostdevblog.blogspot.com/2020/10/screen-space-reflections-in-blightbound.html
//   Finite thickness: the ray must meet the surface slab, not only share a UV.
//
// View space is +Z into the scene (pie_PerspectiveGet). The voxel extends away
// from the camera: [surfZ, surfZ + thickness]. Water (1 - normals.a) and sky
// (depth >= 0.9999) are skipped before the overlap test.

// Empty/sky prepass depth. Same convention as SSAO generate.
const float SKY_DEPTH_THRESHOLD = 0.9999;
const float SSR_WEIGHT_EPSILON = 1e-3;
const float NORMAL_LENGTH_EPSILON = 1e-5;
const float UV_EPSILON = 1e-6;
const float EDGE_FADE_WIDTH = 0.05;

// Loop bound. McGuire's quality floor is ~25 steps at 1080p; 64 matches ssr.h.
// n = min(pixelCount, MAX_STEPS) keeps stride bounded on a mirror.
const int MAX_STEPS = 64;
// First sample sits this far along R so it is not the reflector texel.
const float MIN_RAY_START_ABS = 0.25;
// pie_PerspectiveGet near plane (perspectiveZClose). Clip so clip-w stays valid.
const float NEAR_PLANE_Z = 330.0;
// Slab min(cpuCap, max(MIN, REL * |surfZ|)). At view-Z ~2000 that is ~20 map
// units (droid/hover scale). McGuire Fig. 3: small thickness is strict.
const float THICKNESS_RELATIVE = 0.01;
const float THICKNESS_MIN = 8.0;
// Fewer than half a generate pixel of UV travel => no screen-space walk.
const float MIN_SCREEN_TRAVEL_PX = 0.5;
// Refine the last segment 16x via 3D mix(lastMiss, hitView), not a second DDA.
const int BINARY_SEARCH_STEPS = 4;

// Hit 0.75-1.0 vs miss 0.3-0.65: the gap is the blur classifier
// (ssr_blur.frag SSR_BLUR_HIT_ALPHA = 0.75).
const float MISS_CONFIDENCE_MIN = 0.3;
const float MISS_CONFIDENCE_MAX = 0.65;
const float HIT_CONFIDENCE_MIN = 0.75;
const float HIT_CONFIDENCE_MAX = 1.0;

// Camera-facing fallback. View space is +Z into the scene (pie_PerspectiveGet);
// packed RGB 0.5 unpacks to ~0. Must match SSAO generate.
const vec3 SSR_FALLBACK_VIEW_NORMAL = vec3(0.0, 0.0, -1.0);

bool ssrIsReflectorPixel(vec2 uv)
{
	return (1.0 - texture(normalsTexture, uv).a) > SSR_WEIGHT_EPSILON;
}

// Farther = larger +Z. Thickness is a view-Z slab, not a radial length.
bool ssrRayOverlapsSurface(float rayZMin, float rayZMax, float surfZ, float thickness)
{
	return rayZMax >= surfZ && rayZMin <= surfZ + thickness;
}

float ssrViewThickness(float surfZ, float thicknessCap)
{
	return min(thicknessCap, max(THICKNESS_MIN, abs(surfZ) * THICKNESS_RELATIVE));
}

float ssrClipRayToNearPlane(vec3 origin, vec3 R, float maxDist)
{
	if (R.z < -1e-5)
	{
		float tNear = (NEAR_PLANE_Z - origin.z) / R.z;
		if (tNear > 0.0)
		{
			maxDist = min(maxDist, tNear);
		}
	}
	return maxDist;
}

vec2 clipToUV(vec4 clip)
{
	return vec2(clip.x, -clip.y) * 0.5 + 0.5;
}

vec2 ssrProjectUV(vec3 viewPos)
{
	vec4 clip = projectionMatrix * vec4(viewPos, 1.0);
	clip.xyz /= clip.w;
	return clipToUV(clip);
}

vec3 getViewNormal(vec2 uv)
{
	vec3 n = texture(normalsTexture, uv).xyz * 2.0 - 1.0;
	float len = length(n);
	// Empty or invalid prepass normals must not inject NaNs into the ray direction.
	if (len < NORMAL_LENGTH_EPSILON)
	{
		return SSR_FALLBACK_VIEW_NORMAL;
	}
	return n / len;
}

float edgeFade(vec2 uv, vec2 clampZW)
{
	// Screen-space rays cannot recover data beyond the rendered prepass extent.
	// Fade hits near that boundary instead of exposing a hard reflection cutoff.
	vec2 n = uv / max(clampZW, vec2(UV_EPSILON));
	float fadeX = smoothstep(0.0, EDGE_FADE_WIDTH, uv.x) * smoothstep(1.0, 1.0 - EDGE_FADE_WIDTH, n.x);
	float fadeY = smoothstep(0.0, EDGE_FADE_WIDTH, uv.y) * smoothstep(1.0, 1.0 - EDGE_FADE_WIDTH, n.y);
	return fadeX * fadeY;
}

void writeMiss(float ssrWeight, vec3 N, vec3 V, vec3 R)
{
	// Screen-space color cannot supply sky that is behind the camera or off
	// the framebuffer. A miss looks up the same 2D skybox the ScenePass uses.
	// Keep miss confidence below nearby geometry hits so the blur does not
	// wash units into the sky-colored ripples.
	float ndotv = clamp(dot(N, -V), 0.0, 1.0);
	float confidence = ssrWeight * mix(MISS_CONFIDENCE_MIN, MISS_CONFIDENCE_MAX, ndotv);
	if (skyboxAvailable < 0.5)
	{
		FragColor = vec4(0.0);
		return;
	}
	FragColor = vec4(wzSampleSkyRadiance(R), confidence);
}

void main()
{
	// Scale into the populated part of a potentially padded prepass texture.
	vec2 uv = clamp(texCoords * prepassUvScaleClamp.xy, vec2(0.0), prepassUvScaleClamp.zw);
	float depth = texture(depthTexture, uv).r;
	if (depth >= SKY_DEPTH_THRESHOLD)
	{
		FragColor = vec4(0.0);
		return;
	}

	// Prepass normal alpha stores SSAO weight; its inverse identifies SSR-eligible water.
	float ssrWeight = 1.0 - texture(normalsTexture, uv).a;
	if (ssrWeight < SSR_WEIGHT_EPSILON)
	{
		FragColor = vec4(0.0);
		return;
	}

	vec3 origin = wzGetViewPosition(uv, depth, invProjectionMatrix);
	vec3 N = getViewNormal(uv);
	// In view space the camera is at the origin, so V points camera -> surface.
	vec3 V = normalize(origin);
	// Reject back-facing or malformed normals before reflecting V about them.
	if (dot(N, -V) < 0.0)
	{
		FragColor = vec4(0.0);
		return;
	}

	vec3 R = reflect(V, N);
	float maxDist = ssrClipRayToNearPlane(origin, R, max(params.x, 1.0));
	if (maxDist <= MIN_RAY_START_ABS)
	{
		writeMiss(ssrWeight, N, V, R);
		return;
	}

	vec3 rayOrig = origin + R * MIN_RAY_START_ABS;
	vec3 rayEnd = origin + R * maxDist;
	vec4 H0 = projectionMatrix * vec4(rayOrig, 1.0);
	vec4 H1 = projectionMatrix * vec4(rayEnd, 1.0);
	float k0 = 1.0 / H0.w;
	float k1 = 1.0 / H1.w;
	vec2 uv0 = clipToUV(vec4(H0.xyz * k0, 1.0));
	vec2 uv1 = clipToUV(vec4(H1.xyz * k1, 1.0));
	// Homogeneous interpolators: k = 1/w, Q = view * k, rayView = Q/k.
	vec3 Q0 = rayOrig * k0;
	vec3 Q1 = rayEnd * k1;

	vec2 pixelUV = max(max(abs(dFdx(uv)), abs(dFdy(uv))), vec2(UV_EPSILON));
	float pixelCount = length((uv1 - uv0) / pixelUV);
	if (pixelCount < MIN_SCREEN_TRAVEL_PX)
	{
		writeMiss(ssrWeight, N, V, R);
		return;
	}

	int n = int(min(pixelCount + 0.5, float(MAX_STEPS)));
	n = max(n, 1);

	float thicknessCap = max(params.y, THICKNESS_MIN);
	float prevZ = rayOrig.z;
	// lastMiss and hitView form the bracket later refined by binary search.
	vec3 lastMiss = rayOrig;
	vec3 hitView = rayOrig;
	vec2 hitUV = uv;
	float hitT = MIN_RAY_START_ABS;
	bool hit = false;

	for (int i = 1; i <= MAX_STEPS; ++i)
	{
		if (i > n)
		{
			break;
		}
		float ddaT = float(i) / float(n);
		float k = mix(k0, k1, ddaT);
		vec3 rayView = mix(Q0, Q1, ddaT) / max(k, 1e-8);
		vec2 sampleUV = ssrProjectUV(rayView);
		if (sampleUV.x < 0.0 || sampleUV.y < 0.0 || sampleUV.x > prepassUvScaleClamp.z || sampleUV.y > prepassUvScaleClamp.w)
		{
			break;
		}
		sampleUV = clamp(sampleUV, vec2(0.0), prepassUvScaleClamp.zw);

		float rayZMin = min(prevZ, rayView.z);
		float rayZMax = max(prevZ, rayView.z);
		prevZ = rayView.z;

		float sampleDepth = texture(depthTexture, sampleUV).r;
		if (sampleDepth >= SKY_DEPTH_THRESHOLD)
		{
			lastMiss = rayView;
			continue;
		}
		vec3 surfView = wzGetViewPosition(sampleUV, sampleDepth, invProjectionMatrix);
		// Water is the reflector, not a reflectee. Keep marching.
		if (ssrIsReflectorPixel(sampleUV))
		{
			lastMiss = rayView;
			continue;
		}
		if (ssrRayOverlapsSurface(rayZMin, rayZMax, surfView.z, ssrViewThickness(surfView.z, thicknessCap)))
		{
			hit = true;
			hitView = rayView;
			hitUV = sampleUV;
			hitT = length(rayView - origin);
			break;
		}
		lastMiss = rayView;
	}

	if (!hit)
	{
		writeMiss(ssrWeight, N, V, R);
		return;
	}

	// Refine the coarse first crossing without increasing the primary step count.
	for (int b = 0; b < BINARY_SEARCH_STEPS; ++b)
	{
		vec3 midView = mix(lastMiss, hitView, 0.5);
		vec2 sampleUV = clamp(ssrProjectUV(midView), vec2(0.0), prepassUvScaleClamp.zw);
		float sampleDepth = texture(depthTexture, sampleUV).r;
		if (sampleDepth >= SKY_DEPTH_THRESHOLD)
		{
			lastMiss = midView;
			continue;
		}
		vec3 surfView = wzGetViewPosition(sampleUV, sampleDepth, invProjectionMatrix);
		if (ssrIsReflectorPixel(sampleUV))
		{
			lastMiss = midView;
			continue;
		}
		if (ssrRayOverlapsSurface(min(lastMiss.z, midView.z), max(lastMiss.z, midView.z),
			surfView.z, ssrViewThickness(surfView.z, thicknessCap)))
		{
			hitView = midView;
			hitUV = sampleUV;
			hitT = length(midView - origin);
		}
		else
		{
			lastMiss = midView;
		}
	}

	// Geometry hits need to outrank the water's own ripple albedo. Distance still
	// fades far hits; facing weight stays in compose so this alpha can stay high.
	float confidence = ssrWeight
		* mix(HIT_CONFIDENCE_MIN, HIT_CONFIDENCE_MAX, 1.0 - clamp(hitT / max(maxDist, UV_EPSILON), 0.0, 1.0))
		* edgeFade(hitUV, prepassUvScaleClamp.zw);

	// Convert from prepass allocation coordinates back through logical screen UV
	// into the populated extent of the opaque scene-color texture.
	vec2 sceneUv = clamp(hitUV / max(prepassUvScaleClamp.xy, vec2(UV_EPSILON)) * sceneUvScaleClamp.xy,
		vec2(0.0), sceneUvScaleClamp.zw);
	vec3 color = texture(sceneTexture, sceneUv).rgb;
	FragColor = vec4(color, confidence);
}
