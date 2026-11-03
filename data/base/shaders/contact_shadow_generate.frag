// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	mat4 invProjectionMatrix;
	mat4 projectionMatrix;
	mat4 viewMatrix;
	vec4 sunDirWorld;
	vec4 params;              // x=maxRayLength, y=thicknessCap, z=nearPlane, w=rayBias
	vec4 fadeParams;          // x=minRayStart, y=fadeStartViewZ, z=fadeEndViewZ, w=edgeFadeWidth
	vec4 generatePixelUV;     // xy = 1 / generate viewport
	vec4 prepassUvScaleClamp;
	vec2 projZCoeffs;
	float stepCount;
	float padding;
};

uniform sampler2D depthTexture;
uniform sampler2D normalsTexture;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex, uv) texture2D(tex, uv)
#endif

#ifdef NEWGL
in vec2 texCoords;
out vec4 FragColor;
#else
varying vec2 texCoords;
#endif

#include "view_position.glsl"
#include "screen_space_ray.glsl"

const int MAX_STEPS = 32;
const float MIN_SCREEN_TRAVEL_PX = 0.5;
const float WEIGHT_EPS = 1e-3;
const float THICKNESS_RELATIVE = 0.008;
const float THICKNESS_MIN = 4.0;
// Same-triangle residual after the geometric plane is unit length. Above float
// noise on view positions of a few thousand, below a droid.
const float PLANE_EPS = 1.5;
// Shading-normal fallback when neither neighbor texel stays on the receiver
// (cliff lip, sky). 0.5 world units of gap per unit marched covers
// angle-weighted normal drift (src/terrain.cpp getGridNormal) without filling
// the facet.
const float PLANE_EPS_FALLBACK = 4.0;
const float PLANE_FALLBACK_SLOPE = 0.5;
// A continuous face moves about one depth-texel footprint per tap. A lip is
// many times that. 80 degrees of slope is about 6 footprints, so 8 keeps the
// face and drops the discontinuity.
const float PLANE_TAP_SCALE = 8.0;
// At homogeneous t = 1 the shadow is lifted 25% of the way back to lit.
const float RAY_FADE = 0.25;

// Viewport UV is 0..1 across the rendered sub-rect (texCoords, and wzClipToUV).
// Texture UV is that rect inside a target that may be larger. prepassUvScaleClamp
// is (used/tex, (used-0.5)/tex): xy scales viewport UV onto the texture, zw is
// the last safe texture UV. Dynamic resolution and render scale both change xy.
vec2 csTexUv(vec2 viewportUv)
{
	return clamp(viewportUv * prepassUvScaleClamp.xy, vec2(0.0), prepassUvScaleClamp.zw);
}

// Depth texel -> viewport UV. Unproject with this, never with the texture UV.
vec2 csViewportUv(ivec2 coord, ivec2 depthSize)
{
	vec2 texUv = (vec2(coord) + 0.5) / vec2(depthSize);
	return texUv / max(prepassUvScaleClamp.xy, vec2(WZ_SS_UV_EPS));
}

// Explicit depth taps, not dFdx/dFdy. Each invocation fetches its own texels.
// coord is limited to the used sub-rect, so padding outside the dynamic-resolution
// viewport is never read.
bool csReceiverNeighbor(ivec2 coord, ivec2 depthSize, ivec2 usedSize, vec3 surfacePos, float reach, out vec3 pos)
{
	if (coord.x < 0 || coord.y < 0 || coord.x >= usedSize.x || coord.y >= usedSize.y)
	{
		return false;
	}
	float d = texelFetch(depthTexture, coord, 0).r;
	if (d >= WZ_SS_SKY_DEPTH)
	{
		return false;
	}
	pos = wzGetViewPosition(csViewportUv(coord, depthSize), d, invProjectionMatrix);
	return length(pos - surfacePos) <= reach;
}

// +step first, then -step, so the last pixel of a triangle still builds the
// plane from a texel inside that triangle. Two separate ifs: an out parameter
// on a short-circuit || is not worth a driver quirk.
bool csPickAxis(ivec2 base, ivec2 step, ivec2 depthSize, ivec2 usedSize, vec3 surfacePos, float reach, out vec3 offset)
{
	vec3 pos;
	if (csReceiverNeighbor(base + step, depthSize, usedSize, surfacePos, reach, pos))
	{
		offset = pos - surfacePos;
		return true;
	}
	if (csReceiverNeighbor(base - step, depthSize, usedSize, surfacePos, reach, pos))
	{
		offset = pos - surfacePos;
		return true;
	}
	return false;
}

void writeVis(float v)
{
#ifdef NEWGL
	FragColor = vec4(v);
#else
	gl_FragColor = vec4(v);
#endif
}

// McGuire & Mara, JCGT 3(4), 2014: the Z slab is the occluder thickness, and Q/k
// steps stay as they are. The receiver plane is there because a nearest-depth
// slab treats each cliff triangle as an occluder (Stachowiak's "duplo" depth
// buffer). Build it from three texelFetch positions, not from dFdx/dFdy.
// Absolute distance from that plane rejects the same triangle.
// Do not reject rays with L.z < 0. The sun is often back toward the camera,
// and that march is a real contact.
// Viewport UV (0..1 across the rendered sub-rect) is what wzGetViewPosition and
// wzClipToUV speak. Texture UV is viewportUv * prepassUvScaleClamp.xy, clamped
// to zw. Mixing them crops the march when dynamic resolution is below 1.
void main()
{
	// texCoords is viewport UV. Depth and normals live in texture UV.
	vec2 viewportUv = texCoords;
	vec2 texUv = csTexUv(viewportUv);
	float depth = texture(depthTexture, texUv).r;
	if (depth >= WZ_SS_SKY_DEPTH)
	{
		writeVis(1.0);
		return;
	}
	vec3 surfacePos = wzGetViewPosition(viewportUv, depth, invProjectionMatrix);
	float recvWeight = texture(normalsTexture, texUv).a;
	if (recvWeight < WEIGHT_EPS)
	{
		writeVis(1.0);
		return;
	}

	vec3 N = texture(normalsTexture, texUv).xyz * 2.0 - 1.0;
	float nlen = length(N);
	N = (nlen < 1e-5) ? vec3(0.0, 0.0, -1.0) : N / nlen;

	vec3 L = normalize(mat3(viewMatrix) * sunDirWorld.xyz);
	float NdotL = dot(N, L);
	if (NdotL <= 0.0)
	{
		writeVis(1.0);
		return;
	}

	// Geometric plane of the rasterized triangle from the center depth texel
	// and one in-plane neighbor on each axis. Shading N stays the bias /
	// NdotL / fallback axis. Flip so G points toward the camera (visible
	// surfaces have G.z < 0 in this +Z view space).
	// P[0][0] = 330/xangle with xangle = videoBufferWidth/6. NDC X maps onto the
	// used prepass width, not the allocated texture width, so a dynamic-resolution
	// fraction of 0.5 makes one texel twice as wide. generatePixelUV is the
	// contact-pass pixel and must not be used here (Low/Normal are coarser than
	// the prepass).
	ivec2 depthSize = textureSize(depthTexture, 0);
	ivec2 usedSize = max(ivec2(prepassUvScaleClamp.xy * vec2(depthSize) + 0.5), ivec2(1));
	ivec2 base = ivec2(clamp(floor(texUv * vec2(depthSize)), vec2(0.0), vec2(usedSize) - vec2(1.0)));
	float texelWorld = abs(surfacePos.z) * 2.0
		/ (float(usedSize.x) * max(abs(projectionMatrix[0][0]), 1e-4));
	float reach = max(texelWorld, 1e-3) * PLANE_TAP_SCALE;
	vec3 axisX;
	vec3 axisY;
	vec3 G = vec3(0.0, 0.0, -1.0);
	bool planeSmooth = csPickAxis(base, ivec2(1, 0), depthSize, usedSize, surfacePos, reach, axisX)
		&& csPickAxis(base, ivec2(0, 1), depthSize, usedSize, surfacePos, reach, axisY);
	if (planeSmooth)
	{
		vec3 Graw = cross(axisX, axisY);
		float planeArea = length(Graw);
		planeSmooth = planeArea > 1e-4;
		if (planeSmooth)
		{
			G = Graw / planeArea;
			if (G.z > 0.0)
			{
				G = -G;
			}
			planeSmooth = G.z < -0.02;
		}
	}

	// Bias along the shading normal. Keep surfacePos unbiased for the plane.
	vec3 rayPos = surfacePos + N * params.w;
	float maxDist = wzClipRayToNearPlane(rayPos, L, max(params.x, 1.0), params.z);
	if (maxDist <= fadeParams.x)
	{
		writeVis(1.0);
		return;
	}

	vec3 rayOrig = rayPos + L * fadeParams.x;
	vec3 rayEnd = rayPos + L * maxDist;
	WzHomogeneousRay ray = wzMakeHomogeneousRay(projectionMatrix, rayOrig, rayEnd);
	vec2 pixelUV = max(generatePixelUV.xy, vec2(WZ_SS_UV_EPS));
	float pixelCount = length((ray.uv1 - ray.uv0) / pixelUV);
	if (pixelCount < MIN_SCREEN_TRAVEL_PX)
	{
		writeVis(1.0);
		return;
	}

	int n = clamp(int(min(pixelCount + 0.5, stepCount)), 1, MAX_STEPS);
	// jitter in [0, 1). Every step uses it, so t increases.
	//   i = 1 -> about [0.5/n, 1/n]
	//   i = 2 -> (1 + jitter) / n  which is always greater
	// viewportUv / generatePixelUV is the generate-pass pixel. generatePixelUV
	// is already 1/usedWriteViewport, so dynamic resolution does not rescale it.
	float jitter = wzInterleavedGradientNoise(viewportUv / pixelUV);

	float prevZ = rayOrig.z;
	bool hit = false;
	float hitT = 0.0;
	vec2 hitViewportUv = viewportUv;

	for (int i = 1; i <= MAX_STEPS; ++i)
	{
		if (i > n)
		{
			break;
		}
		float ddaT = max((float(i) - 1.0 + jitter) / float(n), 0.5 / float(n));
		if (ddaT > 1.0)
		{
			break;
		}
		vec3 rayView = wzHomogeneousView(ray, ddaT);
		// Homogeneous UV is viewport UV (wzClipToUV). Comparing it to
		// prepassUvScaleClamp.zw (texture UV, about `fraction`) would end the
		// ray partway across the image whenever dynamic resolution is below 1.
		vec2 sampleViewportUv = wzHomogeneousUV(ray, ddaT);
		if (sampleViewportUv.x < 0.0 || sampleViewportUv.y < 0.0
			|| sampleViewportUv.x > 1.0 || sampleViewportUv.y > 1.0)
		{
			break;
		}
		vec2 sampleTexUv = csTexUv(sampleViewportUv);

		float rayZMin = min(prevZ, rayView.z);
		float rayZMax = max(prevZ, rayView.z);
		prevZ = rayView.z;

		float sampleDepth = texture(depthTexture, sampleTexUv).r;
		if (sampleDepth >= WZ_SS_SKY_DEPTH)
		{
			continue;
		}
		float surfZ = wzGetViewZ(sampleDepth, projZCoeffs);
		float thickness = wzViewThickness(surfZ, params.y, THICKNESS_RELATIVE, THICKNESS_MIN);
		// Pen is the sample's own view Z, not rayZMax. rayZMax lets the far
		// end of a long step confirm a hit while this texel is still in front.
		float pen = rayView.z - surfZ;
		if (!(pen > 0.0 && pen <= thickness
			&& wzRayOverlapsSurface(rayZMin, rayZMax, surfZ, thickness)))
		{
			continue;
		}

		vec3 samplePos = wzGetViewPosition(sampleViewportUv, sampleDepth, invProjectionMatrix);
		float marched = max(length(rayView - surfacePos), 1.0);
		float eps = planeSmooth ? PLANE_EPS : max(PLANE_EPS_FALLBACK, marched * PLANE_FALLBACK_SLOPE);
		vec3 axis = planeSmooth ? G : N;
		if (abs(dot(samplePos - surfacePos, axis)) <= eps)
		{
			continue;
		}

		hit = true;
		hitT = ddaT;
		hitViewportUv = sampleViewportUv;
		break;
	}

	if (!hit)
	{
		writeVis(1.0);
		return;
	}

	// Each factor is 1 for a confident hit and 0 when the hit should disappear.
	// mix(lit, shadowed, factor) therefore returns to white at distance, at the
	// screen edge, and at the far end of the ray.
	// Width is a fraction of the rendered image. Passing texture UV and zw
	// fades the left edge in absolute texture units, which grows when the
	// used rect shrinks. Viewport UV with a (1, 1) clamp keeps 5% of the image.
	float edgeFade = wzScreenEdgeFade(viewportUv, vec2(1.0), fadeParams.w)
		* wzScreenEdgeFade(hitViewportUv, vec2(1.0), fadeParams.w);
	float distFade = 1.0 - smoothstep(fadeParams.y, fadeParams.z, abs(surfacePos.z));
	float rayFade = 1.0 - clamp(hitT, 0.0, 1.0) * RAY_FADE;
	float fade = clamp(edgeFade * distFade * rayFade, 0.0, 1.0);
	writeVis(mix(1.0, 0.0, fade));
}
