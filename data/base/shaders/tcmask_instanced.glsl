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
	float pad0;
	float pad1;
	float pad2;
};

layout(std140) uniform meshuniforms {
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
	int shieldEffect;
};

layout(std140) uniform pointlights {
	vec4 PointLightsPosition[WZ_MAX_POINT_LIGHTS];
	vec4 PointLightsColorAndEnergy[WZ_MAX_POINT_LIGHTS];
	ivec4 bucketOffsetAndSize[WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION];
	ivec4 PointLightsIndex[WZ_MAX_INDEXED_POINT_LIGHTS];
	int bucketDimensionUsed;
};
