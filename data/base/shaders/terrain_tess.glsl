// Shared tessellation math for the hardware-tessellated terrain strategy.
// Every terrain TES variant (color, depth prepass, shadow/depth map) includes
// this file: the position expressions MUST stay identical between them so all
// passes rasterize the exact same surface.
//
// Field texture encodings must match src/terrain_bake.cpp - every encoding is
// a linear map, so hardware bilinear filtering of the encoded texels filters
// the decoded fields exactly:
//   height (R16_UNORM):  texel = (height + 2048) * 16 / 65535
//   offset (RG16_UNORM): texel = (offset + 64) / 128
//   normal (RG8_UNORM):  texel = normal.xz * 0.5 + 0.5
// One texel per bake-grid point. World (x, y) = texel * WZ_BAKE_STEP.

const float WZ_TESS_LEVEL = 4.0; // fixed uniform tessellation level
const float WZ_TILE_UNITS = 128.0;
const float WZ_BAKE_SAMPLES_PER_TILE = 8.0; // must match terrain_bake.h
const float WZ_BAKE_STEP = WZ_TILE_UNITS / WZ_BAKE_SAMPLES_PER_TILE;

// Evaluate the baked terrain surface at an (unwarped) lattice point, given in
// world units. Outputs the model-space position (with the horizontal outline
// warp applied) and the model-space normal.
void wz_evalTerrain(in sampler2D heightTex, in sampler2D offsetTex, in sampler2D normalTex, in vec2 latticeXY,
					out vec3 posModel, out vec3 normalModel)
{
	// bake texel (i, j) holds the field at world (i, j) * WZ_BAKE_STEP. Sample
	// at texel centers so hardware bilinear interpolates between lattice points
	vec2 uv = (latticeXY / WZ_BAKE_STEP + 0.5) / vec2(textureSize(heightTex, 0));
	float h = textureLod(heightTex, uv, 0.0).r * (65535.0 / 16.0) - 2048.0;
	vec2 off = textureLod(offsetTex, uv, 0.0).rg * WZ_TILE_UNITS - (WZ_TILE_UNITS * 0.5);
	vec2 nxz = textureLod(normalTex, uv, 0.0).rg * 2.0 - 1.0;

	// renderer model space: x = world x, z = -world y, +y up
	// (same expression as the CPU subdivided builder: heights/normals from the
	// undisplaced lattice, the offset warps the outline horizontally)
	posModel = vec3(latticeXY.x + off.x, h, -(latticeXY.y + off.y));
	float ny = sqrt(max(1.0 - dot(nxz, nxz), 0.0001));
	normalModel = normalize(vec3(nxz.x, ny, nxz.y));
}

// Patch corners arrive via the patch index buffer in the order
// [c00, c10, c11, c01] (see the terrain patch index build in terrain.cpp).
// gl_TessCoord.x runs along map x, .y along map y.
#define WZ_PATCH_BILINEAR(arr, u, v) \
	mix(mix(arr[0], arr[1], u), mix(arr[3], arr[2], u), v)

// lattice (world x, world y) of the evaluation point from the corner positions
vec2 wz_patchLatticeXY(vec4 p0, vec4 p1, vec4 p2, vec4 p3, vec2 uv)
{
	vec2 c00 = vec2(p0.x, -p0.z);
	vec2 c10 = vec2(p1.x, -p1.z);
	vec2 c11 = vec2(p2.x, -p2.z);
	vec2 c01 = vec2(p3.x, -p3.z);
	return mix(mix(c00, c10, uv.x), mix(c01, c11, uv.x), uv.y);
}
