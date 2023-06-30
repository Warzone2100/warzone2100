#version 450

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;

layout(set = 1, binding = 0) uniform sampler2D lightmap_tex;
layout(set = 1, binding = 1) uniform sampler2D tex2;

layout(std140, set = 0, binding = 0) uniform cbuffer {
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
};

layout(location = 1) in vec2 uvLightmap;
layout(location = 2) in vec2 uv1;
layout(location = 3) in vec2 uv2;
layout(location = 4) in float vertexDistance;
layout(location = 5) in float depth;

layout(location = 0) out vec4 FragColor;

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

vec4 main_classic()
{
	vec4 decal = texture(tex2, uv2, WZ_MIP_LOAD_BIAS);
	vec4 lightmap_vec4 = texture(lightmap_tex, uvLightmap, 0.f);
	vec4 color = decal * vec4(vec3(lightmap_vec4.a), 1.f); // ... * tile brightness / ambient occlusion (stored in lightmap.a);
	color.rgb = blendAddEffectLighting(color.rgb, (lightmap_vec4.rgb / 2.f)); // additive color (from environmental point lights / effects)
	return color;
}

void main()
{
	vec4 fragColor = main_classic();

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, fogColor, fogFactor);
	}

	FragColor = fragColor;
}
