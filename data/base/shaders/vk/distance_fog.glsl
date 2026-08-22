// Shared distance-fog math for deferred opaque and forward transparent rendering.
//
// Opaque pixels have one authoritative prepass depth and are fogged by FogApply.
// Transparent layers do not: each layer must use its own fragment distance before
// blending. Moving FogApply after transparency would use the opaque background
// depth and lose the individual alpha/additive contributions.

const int WZ_FOG_OUTPUT_DISABLED = 0;
const int WZ_FOG_OUTPUT_STRAIGHT_ALPHA = 1;
const int WZ_FOG_OUTPUT_PREMULTIPLIED = 2;
const int WZ_FOG_OUTPUT_ADDITIVE = 3;

float wzDistanceFogAmount(float viewDistance, float fogBegin, float fogEnd)
{
	return clamp((viewDistance - fogBegin) / max(fogEnd - fogBegin, 1e-6), 0.0, 1.0);
}

vec3 wzApplyForwardFog(vec3 rgb, float alpha, float fogAmount, vec3 fogColor, int outputMode)
{
	float transmittance = 1.0 - fogAmount;
	if (outputMode == WZ_FOG_OUTPUT_STRAIGHT_ALPHA)
	{
		return mix(rgb, fogColor, fogAmount);
	}
	if (outputMode == WZ_FOG_OUTPUT_PREMULTIPLIED)
	{
		return rgb * transmittance + fogColor * alpha * fogAmount;
	}
	if (outputMode == WZ_FOG_OUTPUT_ADDITIVE)
	{
		// The fogged background already contains atmospheric in-scattering.
		// Additive emission contributes only the light that reaches the camera.
		return rgb * transmittance;
	}
	return rgb;
}
