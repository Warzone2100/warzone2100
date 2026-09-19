#version 450

layout(set = 1, binding = 0) uniform sampler2D tex1;
layout(set = 1, binding = 1) uniform sampler2D tex2;
layout(set = 1, binding = 2) uniform sampler2D lightmap_tex;

layout(std140, set = 0, binding = 0) uniform cbuffer {
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

layout(location = 1) in vec4 uv1_uv2;
layout(location = 2) in vec2 uvLightmap;
layout(location = 3) in float vertexDistance;
layout(location = 4) in vec3 lightDir;
layout(location = 5) in vec3 halfVec;
layout(location = 6) in float depth;
layout(location = 7) in float depth2;

layout(location = 0) out vec4 FragColor;

vec4 main_medium()
{
	vec2 uv1 = uv1_uv2.xy;
	vec2 uv2 = uv1_uv2.zw;
	vec4 fragColor = texture(tex1, uv1, WZ_MIP_LOAD_BIAS);
	float specColor = texture(tex2, uv2, WZ_MIP_LOAD_BIAS).r;
	fragColor *= vec4(specColor, specColor, specColor, 1.0);
	return fragColor;
}

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

void main()
{
	vec4 fragColor = main_medium();

	FragColor = fragColor;
}
