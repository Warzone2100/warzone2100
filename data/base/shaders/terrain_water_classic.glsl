// Constants shared by the classic water vertex and fragment stages.
// Member order must match gfx_api::constant_buffer_type<SHADER_WATER_CLASSIC> exactly.

layout(std140) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUVLightmapMatrix;
	mat4 ShadowMapMVPMatrix;
	mat4 ModelUV1Matrix;
	mat4 ModelUV2Matrix;
	vec4 cameraPos; // in modelSpace
	vec4 sunPos; // in modelSpace, normalized
	vec4 fogColor;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	float timeSec;
	float WZ_MIP_LOAD_BIAS;
};
