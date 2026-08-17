// common parts of terrain_water_high.vert and .frag shaders

#include "wz_shader_constants.glsl"

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ModelUVLightmapMatrix;
	mat4 ShadowMapMVPMatrix[WZ_MAX_SHADOW_CASCADES];
	vec4 cameraPos; // in modelSpace
	vec4 sunPos; // in modelSpace, normalized
	vec4 emissiveLight; // light colors/intensity
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
	vec4 ShadowMapCascadeSplits;
	int ShadowMapSize;
	float timeSec;
	float WZ_MIP_LOAD_BIAS;
	float pad0;
	int viewportWidth;
	int viewportHeight;
	int bucketDimensionUsed;
	float pad1;
	// Last because its length follows the grid dimension
	ivec4 bucketOffsetAndSize[WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION];
};

// Light data shares the texture set rather than taking one of its own, because the instanced
// mesh pipeline already sits on the four bound sets Vulkan guarantees. The two consumers do not
// agree on which set that is, so each names its own.
#define WZ_LIGHT_DATA_SET 2

// Only the uniform block transport keeps the light arrays here.
// An empty block is not legal, so the whole declaration goes rather than its contents.
#if WZ_LIGHT_TRANSPORT == 0
layout(std140, set = 1, binding = 0) uniform pointlights {
	vec4 PointLightsPosition[WZ_MAX_POINT_LIGHTS];
	vec4 PointLightsColorAndEnergy[WZ_MAX_POINT_LIGHTS];
	vec4 PointLightsDirectionAndCos[WZ_MAX_POINT_LIGHTS];
	ivec4 PointLightsIndex[WZ_MAX_INDEXED_POINT_LIGHTS];
};
#endif

// interpolated data. location count = 2
struct FragData {
	// for Shadows
	vec3 posModelSpace;
	vec3 posViewSpace;
};
