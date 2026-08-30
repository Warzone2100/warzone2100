// The sRGB transfer function.
// While linear lighting is on the hardware decodes color textures on sample. Values that don't come from a
// color texture are converted with these helpers.
//
// Display values are the gamma encoded colors shown on screen. The lighting formulas were tuned for display
// values, and sRGB is not a power curve, so converting each factor of such a product to linear on its own
// gives a darker result where the values are dark. Sums, mixes and fades have the same problem. So a shader
// using such a formula runs it in display space and converts the result back to linear once. Point lights
// have no display space formula and are added in linear afterward.

vec3 wzLinearToSrgb(vec3 color)
{
	vec3 lo = color * 12.92;
	vec3 hi = 1.055 * pow(max(color, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;
	return mix(hi, lo, step(color, vec3(0.0031308)));
}

vec3 wzSrgbToLinear(vec3 color)
{
	vec3 lo = color / 12.92;
	vec3 hi = pow((max(color, vec3(0.0)) + 0.055) / 1.055, vec3(2.4));
	return mix(hi, lo, step(color, vec3(0.04045)));
}
