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

// Defined in srgb.glsl, which may be included after this header
vec3 wzLinearToSrgb(vec3 color);
vec3 wzSrgbToLinear(vec3 color);

// The fog color and fade were tuned for display values, so under linear lighting fog mixes in display space
vec3 wzApplyForwardFog(vec3 rgb, float alpha, float fogAmount, vec3 fogColor, int outputMode, int linearLighting)
{
	if (outputMode == WZ_FOG_OUTPUT_DISABLED)
	{
		return rgb;
	}
	float transmittance = 1.0 - fogAmount;
	vec3 color = rgb;
	vec3 fog = fogColor;
	if (linearLighting != 0)
	{
		color = wzLinearToSrgb(rgb);
		fog = wzLinearToSrgb(fogColor);
	}
	vec3 result = color;
	if (outputMode == WZ_FOG_OUTPUT_STRAIGHT_ALPHA)
	{
		result = mix(color, fog, fogAmount);
	}
	else if (outputMode == WZ_FOG_OUTPUT_PREMULTIPLIED)
	{
		result = color * transmittance + fog * alpha * fogAmount;
	}
	else if (outputMode == WZ_FOG_OUTPUT_ADDITIVE)
	{
		// The fogged background already contains atmospheric in-scattering.
		// Additive emission contributes only the light that reaches the camera.
		result = color * transmittance;
	}
	if (linearLighting != 0)
	{
		result = wzSrgbToLinear(result);
	}
	return result;
}
