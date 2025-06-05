// common parts of terrain_water_high.vert and .frag shaders

#define WZ_MAX_SHADOW_CASCADES 3

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
	vec4 fogColor;
	vec4 ShadowMapCascadeSplits;
	int ShadowMapSize;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	float timeSec;
};

// interpolated data. location count = 1
struct FragData {
	// for Shadows
	vec3 fragPos;
	// vec3 fragNormal;
};
