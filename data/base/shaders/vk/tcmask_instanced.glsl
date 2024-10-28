// common parts of tcmask/nolight_instanced.vert and .frag shaders

#define WZ_MAX_SHADOW_CASCADES 3

#define WZ_MAX_POINT_LIGHTS 128
#define WZ_MAX_INDEXED_POINT_LIGHTS 512
#define WZ_BUCKET_DIMENSION 8

layout(std140, set = 0, binding = 0) uniform globaluniforms
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ModelUVLightmapMatrix;
	mat4 ShadowMapMVPMatrix[WZ_MAX_SHADOW_CASCADES];
	vec4 lightPosition;
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 fogColor;
	vec4 ShadowMapCascadeSplits;
	int ShadowMapSize;
	float fogEnd;
	float fogStart;
	float graphicsCycle;
	int fogEnabled;
	int viewportWidth;
	int viewportHeight;

	vec4 PointLightsPosition[WZ_MAX_POINT_LIGHTS];
	vec4 PointLightsColorAndEnergy[WZ_MAX_POINT_LIGHTS];
	ivec4 bucketOffsetAndSize[WZ_BUCKET_DIMENSION * WZ_BUCKET_DIMENSION];
	ivec4 PointLightsIndex[WZ_MAX_INDEXED_POINT_LIGHTS];
	int bucketDimensionUsed;
};

layout(std140, set = 1, binding = 0) uniform meshuniforms
{
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
	int shieldEffect;
};

