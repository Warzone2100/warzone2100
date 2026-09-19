// Shared tessellation math for the hardware-tessellated terrain strategy.
// Every terrain TES variant (color, shadow/depth map) includes
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

// MARK: - Adaptive tessellation factors (TCS)

const float WZ_TESS_TARGET_PIXELS = 14.0; // target screen pixels per tessellated segment

// Snap a raw factor to powers of two, with a short fade near the top of each
// band: factors hold constant while the camera moves (no vertex swimming),
// and band transitions animate smoothly (fractional_odd spacing in the TES).
float wz_snapTessFactor(float raw)
{
	float level = max(log2(raw), 0.0);
	float band = floor(level);
	float frac = level - band;
	return exp2(band + smoothstep(0.85, 1.0, frac));
}

// Screen-space edge factor from the edge's two shared corners ONLY (and it is
// symmetric in them), so adjacent patches compute bit-identical factors for a
// shared edge - crack-free across patches, sectors, and passes.
// The factor projects the edge's world LENGTH at the edge's depth rather than
// the projected edge vector: a projected vector shrinks both with distance and
// with foreshortening, and foreshortening must not reduce detail - edge-on
// terrain (a curved shoreline facing away from the camera) sits near the
// silhouette, exactly where coarse geometry is most visible.
float wz_edgeTessFactor(vec3 cornerA, vec3 cornerB, mat4 factorMVP, float viewportHeight, float maxLevel)
{
	float wA = abs((factorMVP * vec4(cornerA, 1.0)).w);
	float wB = abs((factorMVP * vec4(cornerB, 1.0)).w);
	// NDC-per-world-unit projection scale at w == 1, from the factor matrix's
	// y row (the view part is a rigid rotation, so the row keeps the
	// projection's [1][1] as its norm)
	float projScaleY = length(vec3(factorMVP[0][1], factorMVP[1][1], factorMVP[2][1]));
	float pixels = length(cornerB - cornerA) * projScaleY / max(0.5 * (wA + wB), 1.0) * 0.5 * viewportHeight;
	return wz_snapTessFactor(clamp(pixels / WZ_TESS_TARGET_PIXELS, 1.0, maxLevel));
}

// Conservative whole-patch frustum test in the pass's own clip space. Corners
// are inflated outward from the patch center (covers the horizontal outline
// warp) and tested at two vertical extremes (covers the true surface's
// deviation from the legacy corner heights: interpolation overshoot + fillet).
bool wz_patchOutsideFrustum(mat4 passMVP, vec3 c00, vec3 c10, vec3 c11, vec3 c01)
{
	vec3 center = (c00 + c10 + c11 + c01) * 0.25;
	int outsideAll = 0x3F;
	for (int i = 0; i < 4; i++)
	{
		vec3 c = (i == 0) ? c00 : (i == 1) ? c10 : (i == 2) ? c11 : c01;
		c.xz = center.xz + (c.xz - center.xz) * 1.75;
		for (int j = 0; j < 2; j++)
		{
			vec4 p = passMVP * vec4(c.x, c.y + ((j == 0) ? -80.0 : 80.0), c.z, 1.0);
			int outcode = 0;
			if (p.x < -p.w) { outcode |= 1; }
			if (p.x >  p.w) { outcode |= 2; }
			if (p.y < -p.w) { outcode |= 4; }
			if (p.y >  p.w) { outcode |= 8; }
			if (p.z < -p.w) { outcode |= 16; }
			if (p.z >  p.w) { outcode |= 32; }
			outsideAll &= outcode;
			if (outsideAll == 0) { return false; }
		}
	}
	return outsideAll != 0;
}

// Compute the patch's tessellation levels. passMVP is the pass's own
// transform, used for culling. factorMVP is the MAIN camera's transform -
// every pass MUST use the same factorMVP so all passes rasterize identical
// terrain geometry (shadow cascades pass the light's MVP as passMVP but the
// camera's as factorMVP).
void wz_terrainTessLevels(mat4 passMVP, mat4 factorMVP, float viewportHeight, float maxLevel,
						  vec3 c00, vec3 c10, vec3 c11, vec3 c01,
						  out float outerLevels[4], out float innerLevels[2])
{
	if (wz_patchOutsideFrustum(passMVP, c00, c10, c11, c01))
	{
		// a non-positive outer level discards the patch
		outerLevels = float[4](-1.0, -1.0, -1.0, -1.0);
		innerLevels = float[2](-1.0, -1.0);
		return;
	}
	// patch corner order [c00, c10, c11, c01]. gl_TessCoord.x runs c00 -> c10.
	// outer[0] = edge u=0 (c00-c01), [1] = v=0 (c00-c10), [2] = u=1 (c10-c11), [3] = v=1 (c01-c11)
	outerLevels[0] = wz_edgeTessFactor(c00, c01, factorMVP, viewportHeight, maxLevel);
	outerLevels[1] = wz_edgeTessFactor(c00, c10, factorMVP, viewportHeight, maxLevel);
	outerLevels[2] = wz_edgeTessFactor(c10, c11, factorMVP, viewportHeight, maxLevel);
	outerLevels[3] = wz_edgeTessFactor(c01, c11, factorMVP, viewportHeight, maxLevel);
	float innerLevel = max(max(outerLevels[0], outerLevels[1]), max(outerLevels[2], outerLevels[3]));
	innerLevels = float[2](innerLevel, innerLevel);
}
