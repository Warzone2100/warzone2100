// common parts of terrainDecails.vert and .frag shaders

#define WZ_MAX_SHADOW_CASCADES 3

#define WZ_MAX_POINT_LIGHTS 128
#define WZ_MAX_INDEXED_POINT_LIGHTS 512
#define WZ_BUCKET_DIMENSION 8

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
	vec4 fogColor;
	vec4 ShadowMapCascadeSplits;
	int ShadowMapSize;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	int quality;
	int viewportWidth;
	int viewportHeight;
	vec4 PointLightsPosition[WZ_MAX_POINT_LIGHTS];
	vec4 PointLightsColorAndEnergy[WZ_MAX_POINT_LIGHTS];
	ivec4 bucketOffsetAndSize[WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION];
	ivec4 PointLightsIndex[WZ_MAX_INDEXED_POINT_LIGHTS];
	int bucketDimensionUsed;
};

// interpolated data. location count = 11
struct FragData {
	vec2 uvLightmap;
	vec2 uvGround;
	vec2 uvDecal;
	float vertexDistance;
	vec4 groundWeights;
	// In tangent space
	vec3 groundLightDir;
	vec3 groundHalfVec;
	mat2 decal2groundMat2;
	// for Shadows
	vec3 fragPos;
	vec3 fragNormal;
};

// non-interpolated data
struct FragFlatData {
	int tileNo;
	uvec4 grounds;
};
