#ifndef WZ_SCREEN_SPACE_RAY_GLSL
#define WZ_SCREEN_SPACE_RAY_GLSL

const float WZ_SS_SKY_DEPTH = 0.9999;
const float WZ_SS_UV_EPS = 1e-6;

vec2 wzClipToUV(vec4 clip)
{
	return vec2(clip.x, -clip.y) * 0.5 + 0.5;
}

float wzScreenEdgeFade(vec2 uv, vec2 clampZW, float width)
{
	vec2 n = uv / max(clampZW, vec2(WZ_SS_UV_EPS));
	float fadeX = smoothstep(0.0, width, uv.x) * (1.0 - smoothstep(1.0 - width, 1.0, n.x));
	float fadeY = smoothstep(0.0, width, uv.y) * (1.0 - smoothstep(1.0 - width, 1.0, n.y));
	return fadeX * fadeY;
}

float wzViewThickness(float surfZ, float thicknessCap, float relative, float thicknessMin)
{
	return min(thicknessCap, max(thicknessMin, abs(surfZ) * relative));
}

bool wzRayOverlapsSurface(float rayZMin, float rayZMax, float surfZ, float thickness)
{
	return rayZMax >= surfZ && rayZMin <= surfZ + thickness;
}

float wzClipRayToNearPlane(vec3 origin, vec3 R, float maxDist, float nearPlaneZ)
{
	if (R.z < -1e-5)
	{
		float tNear = (nearPlaneZ - origin.z) / R.z;
		if (tNear > 0.0)
		{
			maxDist = min(maxDist, tNear);
		}
	}
	return maxDist;
}

float wzInterleavedGradientNoise(vec2 pixel)
{
	vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
	return fract(magic.z * fract(dot(pixel, magic.xy)));
}

struct WzHomogeneousRay
{
	float k0;
	float k1;
	vec3 Q0;
	vec3 Q1;
	vec2 uv0;
	vec2 uv1;
};

WzHomogeneousRay wzMakeHomogeneousRay(mat4 projection, vec3 rayOrig, vec3 rayEnd)
{
	WzHomogeneousRay r;
	vec4 H0 = projection * vec4(rayOrig, 1.0);
	vec4 H1 = projection * vec4(rayEnd, 1.0);
	r.k0 = 1.0 / H0.w;
	r.k1 = 1.0 / H1.w;
	r.uv0 = wzClipToUV(vec4(H0.xyz * r.k0, 1.0));
	r.uv1 = wzClipToUV(vec4(H1.xyz * r.k1, 1.0));
	r.Q0 = rayOrig * r.k0;
	r.Q1 = rayEnd * r.k1;
	return r;
}

vec3 wzHomogeneousView(WzHomogeneousRay r, float t)
{
	float k = mix(r.k0, r.k1, t);
	return mix(r.Q0, r.Q1, t) / max(k, 1e-8);
}

vec2 wzHomogeneousUV(WzHomogeneousRay r, float t)
{
	return mix(r.uv0, r.uv1, t);
}

#endif
