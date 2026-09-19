// Direction-indexed lookup of the 2D skybox texpage.
// Uses viewToSkyLocal, skyFogColor, and skyboxTexture from the including shader.
// Matches pie_DrawSkybox: four walls share the same strip (u 0..2 per wall),
// v follows the mesh bands (top=0.01, middle=0.85, baseline=0.99).

const float WALL_UV_REPEATS = 2.0;
// pie_Skybox_Init middle; top vertex is y=1.
const float SKY_MESH_MIDDLE_Y = 0.15;
const float SKY_V_TOP = 0.01;
const float SKY_V_MIDDLE = 0.85;
const float SKY_V_BASELINE = 0.99;
const float SKY_LOWER_BAND_Y0 = -0.45;
const float SKY_LOWER_BAND_SPAN = 0.60;

// The skybox is mipmapped. Implicit texture() on fract(u) from divergent SSR
// miss paths picks a low mip at each wall seam (one-pixel column). Force LOD 0
// so the lookup matches pie_DrawSkybox, which also uses the top mip.
vec3 wzSampleSkyboxLod0(vec2 uv)
{
	return textureLod(skyboxTexture, uv, 0.0).rgb;
}

// Mesh U is linear on each wall, not equal-angle. pie_Skybox_Init winding:
// N +Z u=0 at west, E +X u=0 at north, S -Z u=0 at east, W -X u=0 at south.
float wzSkyWallU(vec3 d, vec3 p, vec3 ad)
{
	float u;
	if (ad.z >= ad.x)
	{
		u = (d.z >= 0.0) ? (p.x + 1.0) : (1.0 - p.x);
	}
	else
	{
		u = (d.x >= 0.0) ? (1.0 - p.z) : (p.z + 1.0);
	}
	return fract(u * 0.5) * WALL_UV_REPEATS;
}

vec3 wzSampleSkyRadiance(vec3 viewDir)
{
	vec3 d = mat3(viewToSkyLocal) * viewDir;
	float len = length(d);
	if (len < 1e-5)
	{
		return skyFogColor.rgb;
	}
	d /= len;

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

	// skybox.vert: fog.w = 1 if vertex.y < 0.5 else 0. Verts exist at middle
	// (0.15) and top (1), so the GPU fade is 1 at middle -> 0 at top; below
	// middle every vert is already 1.
	float fogAmt = 0.0;
	if (skyFogColor.a > 0.5)
	{
		fogAmt = 1.0 - clamp((y - SKY_MESH_MIDDLE_Y) / (1.0 - SKY_MESH_MIDDLE_Y), 0.0, 1.0);
	}
	return mix(wzSampleSkyboxLod0(vec2(wzSkyWallU(d, p, ad), v)), skyFogColor.rgb, fogAmt);
}
