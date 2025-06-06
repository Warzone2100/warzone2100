#version 450

#include "terrain_combined.glsl"

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;
layout (constant_id = 1) const uint WZ_SHADOW_MODE = 1;
layout (constant_id = 2) const uint WZ_SHADOW_FILTER_SIZE = 5;
layout (constant_id = 3) const uint WZ_SHADOW_CASCADES_COUNT = 3;
layout (constant_id = 4) const uint WZ_POINT_LIGHT_ENABLED = 0;

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
layout(set = 1, binding = 9) uniform sampler2DArrayShadow shadowMap;

layout(location = 0) in FragData frag;
layout(location = 11) flat in FragFlatData fragf;
layout(location = 14) in mat3 ModelTangentMatrix;

layout(location = 0) out vec4 FragColor;

#include "terrain_combined_frag.glsl"
#include "pointlights.glsl"

vec3 getGroundUv(int i) {
	uint groundNo = fragf.grounds[i];
	return vec3(frag.uvGround * groundScale[groundNo/4u][groundNo%4u], groundNo);
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
	N = mix(normalize(N * 2.f - 1.f), vec3(0.f,0.f,1.f), vec3(float(N == vec3(0.f,0.f,0.f))));
	res.N += N * w;
	res.gloss += texture(groundSpecular, uv, WZ_MIP_LOAD_BIAS).r * w;
}

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return a + b;
}

vec4 doBumpMapping(BumpData b, vec3 groundLightDir, vec3 groundHalfVec) {
	vec3 L = normalize(groundLightDir);
	float lambertTerm = max(dot(b.N, L), 0.0); // diffuse lighting
	// Gaussian specular term computation
	vec3 H = normalize(groundHalfVec);
	float blinnTerm = clamp(dot(b.N, H), 0.f, 1.f);
	blinnTerm = lambertTerm != 0.0 ? blinnTerm : 0.0;
	blinnTerm = pow(blinnTerm, 16.f);
	float visibility = getShadowVisibility();
	vec4 lightmap_vec4 = texture(lightmap_tex, frag.uvLightmap, 0.f);

	float adjustedTileBrightness = pow(lightmap_vec4.a, 2.f-lightmap_vec4.a); // ... * tile brightness / ambient occlusion (stored in lightmap.a)

	vec4 adjustedAmbientLight = ambientLight*lightmap_vec4.a;
	vec4 light = (ambientLight*0.30f + visibility*(adjustedAmbientLight*0.40f + adjustedAmbientLight*lambertTerm*0.30f + diffuseLight*lambertTerm)) * lightmap_vec4.a;
	light.rgb = blendAddEffectLighting(light.rgb, (lightmap_vec4.rgb / 1.4f)); // additive color (from environmental point lights / effects)

	vec4 light_spec = (visibility*specularLight*blinnTerm*lambertTerm) * adjustedTileBrightness;
	light_spec.rgb = blendAddEffectLighting(light_spec.rgb, (lightmap_vec4.rgb / 2.5f)); // additive color (from environmental point lights / effects)
	light_spec *= (b.gloss * b.gloss);

	vec4 res = (b.color*light) + light_spec;

	if (WZ_POINT_LIGHT_ENABLED == 1)
	{
		// point lights
		vec2 clipSpaceCoord = gl_FragCoord.xy / vec2(viewportWidth, viewportHeight);
		res += iterateOverAllPointLights(clipSpaceCoord, frag.fragPos, b.N, normalize(groundHalfVec - groundLightDir), b.color, b.gloss, ModelTangentMatrix);
	}

	// Calculate water murkiness based on non-constant-density-fog, see https://iquilezles.org/articles/fog/
	vec3 c2p = normalize(frag.fragPos - cameraPos.xyz); // camera to point vector
	vec3 c = normalize(cameraPos.xyz);
	float inscatter = 0.007; // in-scattering
	float extinction = 0.001; // extinction
	float murkyFactor = exp(-c.y * extinction) * (1.0 - exp( -(frag.fragPos.y+40.0) * c2p.y * extinction)) / (-c2p.y * extinction);
	float murkiness = inscatter * murkyFactor;
	float waterDeep = 0.003 * murkyFactor;
	murkiness = clamp(murkiness, 0.0, 1.0);
	waterDeep = clamp(waterDeep, 0.0, 1.0);
	res.rgb = mix(res.rgb, (vec3(0.315,0.425,0.475)*vec3(1.0-waterDeep)) * lightmap_vec4.a, murkiness);

	return vec4(res.rgb, b.color.a);
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
		vec3 n_normalized = normalize(n * 2.f - 1.f);
		n = mix(vec3(n_normalized.xy * frag.decal2groundMat2, n_normalized.z), vec3(0.f,0.f,1.f), vec3(float(n == vec3(0.f,0.f,0.f))));
		bump.N = (1-a)*bump.N + a*n;
		bump.gloss = (1-a)*bump.gloss + a*texture(decalSpecular, uv, WZ_MIP_LOAD_BIAS).r;
	}
	return doBumpMapping(bump, frag.groundLightDir, frag.groundHalfVec);
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
		fragColor = mix(fragColor, vec4(fogColor.rgb, fragColor.a), fogFactor);
	}
	FragColor = fragColor;
}
