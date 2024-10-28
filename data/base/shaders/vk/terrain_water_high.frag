#version 450

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;

layout(set = 1, binding = 0) uniform sampler2DArray tex;
layout(set = 1, binding = 1) uniform sampler2DArray tex_nm;
layout(set = 1, binding = 2) uniform sampler2DArray tex_sm;
layout(set = 1, binding = 3) uniform sampler2D lightmap_tex;

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
	vec4 fogColor;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	float timeSec;
};

layout(location = 1) in vec4 uv1_uv2;
layout(location = 2) in vec2 uvLightmap;
layout(location = 3) in float vertexDistance;
layout(location = 4) in vec3 lightDir;
layout(location = 5) in vec3 halfVec;
layout(location = 6) in float depth;
layout(location = 7) in float depth2;

layout(location = 0) out vec4 FragColor;

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

vec4 main_bumpMapping()
{
	vec2 uv1 = uv1_uv2.xy;
	vec2 uv2 = uv1_uv2.zw;

	vec3 N1 = texture(tex_nm, vec3(uv1, 0.f), WZ_MIP_LOAD_BIAS).xzy*vec3(2.f, 2.f, 2.f) + vec3(-1.f, 0.f, -1.f); // y is up in modelSpace
	vec3 N2 = texture(tex_nm, vec3(uv2, 1.f), WZ_MIP_LOAD_BIAS).xzy*vec3(-2.f, 2.f,-2.f) + vec3(1.f, -1.f, 1.f);

	//use RNM blending to mix normal maps properly, see https://blog.selfshadow.com/publications/blending-in-detail/
	vec3 N = normalize(N1 * dot(N1,N2) - N2*N1.y);

	// Light
	float noise = texture(tex, vec3(uv1, 0.f), WZ_MIP_LOAD_BIAS).r * texture(tex, vec3(uv2, 1.f), WZ_MIP_LOAD_BIAS).r;
	float foam = texture(tex_sm, vec3(uv1, 0.f), WZ_MIP_LOAD_BIAS).r * texture(tex_sm, vec3(uv2, 1.f), WZ_MIP_LOAD_BIAS).r;

	float lambertTerm = max(dot(N, lightDir), 0.0);
	float blinnTerm = pow(max(dot(N, halfVec), 0.0), 32.f);

	vec4 fragColor = vec4(0.16,0.26,0.3,1.0)+(noise+foam)*noise*0.5;
	fragColor = fragColor*(ambientLight+diffuseLight*lambertTerm) + specularLight*blinnTerm*(foam*foam+noise);

	vec4 lightmap_vec4 = texture(lightmap_tex, uvLightmap, 0.f);
	vec4 color = fragColor * vec4(vec3(lightmap_vec4.a), 1.f); // ... * tile brightness / ambient occlusion (stored in lightmap.a);
	color.rgb = blendAddEffectLighting(color.rgb, (lightmap_vec4.rgb / 1.5f)); // additive color (from environmental point lights / effects)
	return color;
}

void main()
{
	vec4 fragColor = main_bumpMapping();
	fragColor = mix(fragColor, fragColor-depth, depth);
	fragColor.a = mix(0.25, 1.0, depth2);

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
