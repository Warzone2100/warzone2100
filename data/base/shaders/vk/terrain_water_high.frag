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

	vec3 N1 = texture(tex_nm, vec3(uv2, 0.f), WZ_MIP_LOAD_BIAS).xzy; // y is up in modelSpace
	vec3 N2 = texture(tex_nm, vec3(uv1, 1.f), WZ_MIP_LOAD_BIAS).xzy;
	//use overlay blending to mix normal maps properly
	bvec3 computedN_select = lessThan(N1, vec3(0.5));
	vec3 computedN_multiply = 2.f * N1 * N2;
	vec3 computedN_screen = vec3(1.f) - 2.f * (vec3(1.f) - N1) * (vec3(1.f) - N2);
	vec3 N = mix(computedN_screen, computedN_multiply, vec3(computedN_select));

	N = mix(normalize(N * 2.f - 1.f), vec3(0.f,1.f,0.f), vec3(float(N == vec3(0.f,0.f,0.f))));

	float lambertTerm = max(dot(N, lightDir), 0.0); // diffuse lighting

	// Gaussian specular term computation
	float gloss = texture(tex_sm, vec3(uv1, 0.f), WZ_MIP_LOAD_BIAS).r * texture(tex_sm, vec3(uv2, 1.f), WZ_MIP_LOAD_BIAS).r;
	vec3 H = normalize(halfVec);
	float exponent = acos(dot(H, N)) / (gloss + 0.05);
	float gaussianTerm = exp(-(exponent * exponent));

	vec4 fragColor = (texture(tex, vec3(uv1, 0.f), WZ_MIP_LOAD_BIAS)+texture(tex, vec3(uv2, 1.f), WZ_MIP_LOAD_BIAS)) * (gloss+vec4(0.08,0.13,0.15,1.0));
	fragColor = fragColor*(ambientLight+diffuseLight*lambertTerm) + specularLight*(1.0-gloss)*gaussianTerm*vec4(1.0,0.843,0.686,1.0);
	vec4 lightmap_vec4 = texture(lightmap_tex, uvLightmap, 0.f);
	vec4 color = fragColor * vec4(vec3(lightmap_vec4.a), 1.f); // ... * tile brightness / ambient occlusion (stored in lightmap.a);
	color.rgb = blendAddEffectLighting(color.rgb, (lightmap_vec4.rgb / 1.5f)); // additive color (from environmental point lights / effects)
	return color;
}

void main()
{
	vec4 fragColor = main_bumpMapping();
	fragColor = mix(fragColor, fragColor-depth*0.0007, depth*0.0009);
	fragColor.a = mix(0.25, 1.0, depth2*0.005);

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
