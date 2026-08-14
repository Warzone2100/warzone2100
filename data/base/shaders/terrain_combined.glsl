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
	vec4 fogColor;
	vec4 ShadowMapCascadeSplits;
	int ShadowMapSize;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	int quality;
	int viewportWidth;
	int viewportHeight;
	float tessMaxLevel;
	float WZ_MIP_LOAD_BIAS;
};

layout(std140) uniform pointlights {
	vec4 PointLightsPosition[WZ_MAX_POINT_LIGHTS];
	vec4 PointLightsColorAndEnergy[WZ_MAX_POINT_LIGHTS];
	ivec4 bucketOffsetAndSize[WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION];
	ivec4 PointLightsIndex[WZ_MAX_INDEXED_POINT_LIGHTS];
	int bucketDimensionUsed;
};
