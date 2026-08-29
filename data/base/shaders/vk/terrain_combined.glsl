// common parts of terrainDecails.vert and .frag shaders

#include "wz_shader_constants.glsl"

layout(std140, set = 0, binding = 0) uniform cbuffer {
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
	// active (w of the info vec4). Last shadow map layer.
	mat4 projectileLightShadowMVP;
	vec4 projectileLightShadowInfo;
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


// interpolated data. location count = 10
struct FragData {
	vec2 uvLightmap;
	vec2 uvGround;
	vec2 uvDecal;
	vec4 groundWeights;
	// In tangent space
	vec3 groundLightDir;
	vec3 groundHalfVec;
	mat2 decal2groundMat2;
	// for Shadows
	vec3 posModelSpace;
	vec3 posViewSpace;
};

// non-interpolated data
struct FragFlatData {
	int tileNo;
	uvec4 grounds;
};
