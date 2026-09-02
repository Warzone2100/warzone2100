// Constants shared by the terrain decals vertex, tessellation and fragment stages.
// Member order must match gfx_api::TerrainCombinedUniforms exactly.

#include "wz_shader_constants.glsl"

layout(std140) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ModelUVLightmapMatrix;
	mat4 ShadowMapMVPMatrix[WZ_MAX_SHADOW_CASCADES];
	mat4 groundScale; // array of scales for ground textures, encoded in mat4. scale_i = groundScale[i/4][i%4]
	vec4 cameraPos; // in modelSpace
	vec4 sunPos; // in modelSpace, normalized
	vec4 emissiveLight; // light colors/intensity
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
	vec4 ShadowMapCascadeSplits;
	int ShadowMapSize;
	int quality;
	int viewportWidth;
	int viewportHeight;
	float tessMaxLevel;
	float WZ_MIP_LOAD_BIAS;
	int bucketDimensionUsed;
	float pad1;
	// Biased ortho MVP of the per-frame projectile light shadow and whether it is
	// active. Last shadow map layer.
	mat4 projectileLightShadowMVP;
	bool projectileLightShadowActive;
	// Last because its length follows the grid dimension
	ivec4 bucketOffsetAndSize[WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION];
};

// Which transport carries the light arrays: 0 uniform block, 1 buffer texture, 2 storage buffer.
// Patched at load on OpenGL, fixed at build time on Vulkan.
#define WZ_LIGHT_TRANSPORT 0

// Only the uniform block transport keeps the light arrays here.
// An empty block is not legal, so the whole declaration goes rather than its contents.
#if WZ_LIGHT_TRANSPORT == 0
layout(std140) uniform pointlights {
	vec4 PointLightsPosition[WZ_MAX_POINT_LIGHTS];
	vec4 PointLightsColorAndEnergy[WZ_MAX_POINT_LIGHTS];
	vec4 PointLightsDirectionAndCos[WZ_MAX_POINT_LIGHTS];
	ivec4 PointLightsIndex[WZ_MAX_INDEXED_POINT_LIGHTS];
};
#endif
