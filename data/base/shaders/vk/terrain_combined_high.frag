#version 450

#include "terrain_combined.glsl"

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;

layout(set = 1, binding = 0) uniform sampler2D lightmap_tex;

// ground texture arrays. layer = ground type
layout(set = 1, binding = 1) uniform sampler2DArray groundTex;
layout(set = 1, binding = 2) uniform sampler2DArray groundNormal;
layout(set = 1, binding = 3) uniform sampler2DArray groundSpecular;
layout(set = 1, binding = 4) uniform sampler2DArray groundHeight;

// decal texture arrays. layer = decal tile
layout(set = 1, binding = 5) uniform sampler2DArray decalTex;
layout(set = 1, binding = 6) uniform sampler2DArray decalNormal;
layout(set = 1, binding = 7) uniform sampler2DArray decalSpecular;
layout(set = 1, binding = 8) uniform sampler2DArray decalHeight;

// depth map
layout(set = 1, binding = 9) uniform sampler2DShadow shadowMap;

layout(location = 0) in FragData frag;
layout(location = 12) flat in FragFlatData fragf;

layout(location = 0) out vec4 FragColor;

vec3 getGroundUv(int i) {
	uint groundNo = fragf.grounds[i];
	return vec3(frag.uvGround * groundScale[groundNo/4u][groundNo%4u], groundNo);
}

vec3 getGround(int i) {
	return texture(groundTex, getGroundUv(i), WZ_MIP_LOAD_BIAS).rgb * frag.groundWeights[i];
}

struct BumpData {
	vec4 color;
	vec3 N;
	float gloss;
};

void getGroundBM(int i, inout BumpData res) {
	vec3 uv = getGroundUv(i);
	float w = frag.groundWeights[i];
	res.color += texture(groundTex, uv, WZ_MIP_LOAD_BIAS) * w;
	vec3 N = texture(groundNormal, uv, WZ_MIP_LOAD_BIAS).xyz;
	if (N == vec3(0)) {
		N = vec3(0,0,1);
	} else {
		N = normalize(N * 2 - 1);
	}
	res.N += N * w;
	res.gloss += texture(groundSpecular, uv, WZ_MIP_LOAD_BIAS).r * w;
}

vec4 doBumpMapping(BumpData b, vec3 lightDir, vec3 halfVec) {
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(b.N, L), 0.0); // diffuse lighting
	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float exponent = acos(dot(H, b.N)) / 0.33;
	float gaussianTerm = exp(-(exponent * exponent));

	vec4 res = b.color*(ambientLight + diffuseLight*lambertTerm) + b.gloss*b.gloss*specularLight*gaussianTerm*lambertTerm;

	return vec4(res.rgb, b.color.a);
}

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

vec4 main_bumpMapping() {
	BumpData bump;
	bump.color = vec4(0);
	bump.N = vec3(0);
	bump.gloss = 0;
	getGroundBM(0, bump);
	getGroundBM(1, bump);
	getGroundBM(2, bump);
	getGroundBM(3, bump);

	if (fragf.tileNo >= 0) {
		vec3 uv = vec3(frag.uvDecal, fragf.tileNo);
		vec4 decalColor = texture(decalTex, uv, WZ_MIP_LOAD_BIAS);
		float a = decalColor.a;
		// blend color, normal and gloss with ground ones based on alpha
		bump.color = (1-a)*bump.color + a*vec4(decalColor.rgb, 1);
		vec3 n = texture(decalNormal, uv, WZ_MIP_LOAD_BIAS).xyz;
		if (n == vec3(0)) {
			n = vec3(0,0,1);
		} else {
			n = normalize(n * 2 - 1);
			n = vec3(n.xy * frag.decal2groundMat2, n.z);
		}
		bump.N = (1-a)*bump.N + a*n;
		bump.gloss = (1-a)*bump.gloss + a*texture(decalSpecular, uv, WZ_MIP_LOAD_BIAS).r;
	}
	vec4 lightmap_vec4 = texture(lightmap_tex, frag.uvLightmap);
	vec4 color = doBumpMapping(bump, frag.groundLightDir, frag.groundHalfVec) * vec4(vec3(lightmap_vec4.a), 1.f); // ... * tile brightness / ambient occlusion (stored in lightmap.a);
	color.rgb = blendAddEffectLighting(color.rgb, (lightmap_vec4.rgb / 1.5f)); // additive color (from environmental point lights / effects)
	return color;
}

void main()
{
	vec4 fragColor = main_bumpMapping();

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - frag.vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), fogFactor);
	}
	FragColor = fragColor;
}
