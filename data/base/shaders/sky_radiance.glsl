// Direction-indexed lookup of the 2D skybox texpage.
// Uses viewToSkyLocal, skyFogColor, and skyboxTexture from the including shader.
// Matches pie_DrawSkybox: four walls share the same strip (u 0..2 per 90 deg),
// v follows the mesh bands (top=0.01, middle=0.85, baseline=0.99).

const float PI = 3.14159265;
const float WALL_UV_REPEATS = 2.0;
const float SKY_MESH_MIDDLE_Y = 0.15;
const float SKY_V_TOP = 0.01;
const float SKY_V_MIDDLE = 0.85;
const float SKY_V_BASELINE = 0.99;
const float SKY_LOWER_BAND_Y0 = -0.45;
const float SKY_LOWER_BAND_SPAN = 0.60;
const float SKY_FOG_Y = 0.5;

vec3 wzSampleSkyRadiance(vec3 viewDir)
{
	vec3 d = mat3(viewToSkyLocal) * viewDir;
	float len = length(d);
	if (len < 1e-5)
	{
		return skyFogColor.rgb;
	}
	d /= len;

	float u = fract((atan(d.x, d.z) + PI) / (0.5 * PI)) * WALL_UV_REPEATS;

	vec3 ad = abs(d);
	vec3 p = d / max(max(ad.x, max(ad.y, ad.z)), 1e-6);
	float y = p.y;
	float v;
	if (y > SKY_MESH_MIDDLE_Y)
	{
		v = mix(SKY_V_MIDDLE, SKY_V_TOP, clamp((y - SKY_MESH_MIDDLE_Y) / (1.0 - SKY_MESH_MIDDLE_Y), 0.0, 1.0));
	}
	else
	{
		v = mix(SKY_V_BASELINE, SKY_V_MIDDLE, clamp((y - SKY_LOWER_BAND_Y0) / SKY_LOWER_BAND_SPAN, 0.0, 1.0));
	}

	vec3 color = texture(skyboxTexture, vec2(u, v)).rgb;
	if (skyFogColor.a > 0.5 && y < SKY_FOG_Y)
	{
		color = skyFogColor.rgb;
	}
	return color;
}
