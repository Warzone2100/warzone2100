// Constants shared by the water vertex and fragment stages.
// Member order must match gfx_api::constant_buffer_type<SHADER_WATER> exactly.

layout(std140) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUVLightmapMatrix;
	mat4 ModelUV1Matrix;
	mat4 ModelUV2Matrix;
	vec4 cameraPos; // in modelSpace
	vec4 sunPos; // in modelSpace, normalized
	vec4 emissiveLight; // light colors/intensity
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
	float timeSec;
	float WZ_MIP_LOAD_BIAS;
	float pad0;
	float pad1;
};
