// Constants shared by the instanced model vertex and fragment stages.
// Member order must match gfx_api::Draw3DShapeInstancedGlobalUniforms and
// gfx_api::Draw3DShapeInstancedPerMeshUniforms exactly.

#include "wz_shader_constants.glsl"

layout(std140) uniform globaluniforms {
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ModelUVLightmapMatrix;
	mat4 ShadowMapMVPMatrix[WZ_MAX_SHADOW_CASCADES];
	vec4 cameraPos; // in model space
	vec4 lightPosition; // in world space
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 ShadowMapCascadeSplits;
	int ShadowMapSize;
	float graphicsCycle;
	int viewportWidth;
	int viewportHeight;
	float WZ_MIP_LOAD_BIAS;
	int bucketDimensionUsed;
	float pad1;
	float pad2;
	// Last because its length follows the grid dimension
	ivec4 bucketOffsetAndSize[WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION];
};

layout(std140) uniform meshuniforms {
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
	int shieldEffect;
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
	ivec4 PointLightsIndex[WZ_MAX_INDEXED_POINT_LIGHTS];
};
#endif
