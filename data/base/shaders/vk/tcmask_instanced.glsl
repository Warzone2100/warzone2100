// common parts of tcmask/nolight_instanced.vert and .frag shaders

#include "wz_shader_constants.glsl"

layout(std140, set = 0, binding = 0) uniform globaluniforms
{
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

layout(std140, set = 1, binding = 0) uniform meshuniforms
{
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
	int shieldEffect;
};

// Light data shares the texture set rather than taking one of its own, because the instanced
// mesh pipeline already sits on the four bound sets Vulkan guarantees. The two consumers do not
// agree on which set that is, so each names its own.
#define WZ_LIGHT_DATA_SET 3

layout(std140, set = 2, binding = 0) uniform pointlights {
	ivec4 bucketOffsetAndSize[WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION];
	int bucketDimensionUsed;
#if WZ_LIGHT_TRANSPORT == 0
	vec4 PointLightsPosition[WZ_MAX_POINT_LIGHTS];
	vec4 PointLightsColorAndEnergy[WZ_MAX_POINT_LIGHTS];
	ivec4 PointLightsIndex[WZ_MAX_INDEXED_POINT_LIGHTS];
#endif
};
