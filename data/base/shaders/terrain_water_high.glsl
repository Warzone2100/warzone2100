// Constants shared by the high water vertex and fragment stages.
// Member order must match gfx_api::constant_buffer_type<SHADER_WATER_HIGH> exactly.

#ifndef WZ_MAX_SHADOW_CASCADES
#define WZ_MAX_SHADOW_CASCADES 3
#endif

layout(std140) uniform cbuffer {
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
	float WZ_MIP_LOAD_BIAS;
};
