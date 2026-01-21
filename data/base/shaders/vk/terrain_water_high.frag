#version 450

#include "terrain_water_high.glsl"

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;
layout (constant_id = 1) const uint WZ_SHADOW_MODE = 1;
layout (constant_id = 2) const uint WZ_SHADOW_FILTER_SIZE = 5;
layout (constant_id = 3) const uint WZ_SHADOW_CASCADES_COUNT = 3;

layout(set = 1, binding = 0) uniform sampler2DArray tex;
layout(set = 1, binding = 1) uniform sampler2DArray tex_nm;
layout(set = 1, binding = 2) uniform sampler2DArray tex_sm;
layout(set = 1, binding = 3) uniform sampler2D lightmap_tex;
// depth map
layout(set = 1, binding = 4) uniform sampler2DArrayShadow shadowMap;

layout(location = 1) in vec4 uv1_uv2;
layout(location = 2) in vec2 uvLightmap;
layout(location = 3) in vec3 lightDir;
layout(location = 4) in vec3 halfVec;
layout(location = 5) in float depth;
layout(location = 6) in vec3 eyeVec;
layout(location = 7) in float fresnel;
layout(location = 8) in float fresnel_alpha;
layout(location = 9) in FragData frag;

layout(location = 0) out vec4 FragColor;

#include "terrain_combined_frag.glsl"
#include "light.glsl"

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

vec4 main_bumpMapping()
{
	vec2 uv1 = uv1_uv2.xy;
	vec2 uv2 = uv1_uv2.zw;

	vec3 N1 = texture(tex_nm, vec3(vec2(uv1.x, uv1.y+timeSec*0.04), 0.0), WZ_MIP_LOAD_BIAS).xzy * vec3( 2.0, 2.0, 2.0) + vec3(-1.0, 0.0, -1.0); // y is up in modelSpace
	vec3 N2 = texture(tex_nm, vec3(vec2(uv2.x+timeSec*0.02, uv2.y), 1.0), WZ_MIP_LOAD_BIAS).xzy * vec3(-2.0, 2.0,-2.0) + vec3( 1.0, -1.0, 1.0);
	vec3 N3 = texture(tex_nm, vec3(vec2(uv1.x+timeSec*0.05, uv1.y), 0.0), WZ_MIP_LOAD_BIAS).xzy * 2.0 - 1.0;
	vec3 N4 = texture(tex_nm, vec3(vec2(uv2.x, uv2.y+timeSec*0.03), 1.0), WZ_MIP_LOAD_BIAS).xzy * 2.0 - 1.0;

	//use RNM blending to mix normal maps properly, see https://blog.selfshadow.com/publications/blending-in-detail/
	vec3 RNM = normalize(N1 * dot(N1,N2) - N2*N1.y);
	vec3 Na = mix(N3, N4, 0.5); //more waves to mix
	vec3 N = mix(RNM, Na, 0.5);
	N = normalize(vec3(N.x, N.y * 5.0, N.z)); // 5 is a strength

	// Textures
	float d = mix(depth * 0.1, depth, 0.5);
	float noise = texture(tex, vec3(uv1, 0.0), WZ_MIP_LOAD_BIAS).r * texture(tex, vec3(uv2, 1.0), WZ_MIP_LOAD_BIAS).r;
	float foam = texture(tex_sm, vec3(vec2(uv1.x, uv1.y), 0.0), WZ_MIP_LOAD_BIAS).r;
	foam *= texture(tex_sm, vec3(vec2(uv2.x, uv2.y), 1.0), WZ_MIP_LOAD_BIAS).r;
	foam = (foam+pow(length(N.xz),2.5)*1000.0)*d*d ;
	foam = clamp(foam, 0.0, 0.2);
	vec3 waterColor = vec3(0.18,0.33,0.42);

	// Light
	float diffuseFactor = lambertTerm(N, lightDir);
	float visibility = getShadowVisibility(frag.posModelSpace, frag.posViewSpace, diffuseFactor, 0.0f);
	diffuseFactor = min(diffuseFactor, visibility*diffuseFactor);
	float specularFactor = blinnTerm(N, halfVec, 0.f, 128.f);
	vec3 reflectLight = reflect(-lightDir, N);
	float r = pow(max(dot(reflectLight, halfVec), 0.0), 14.0);
	specularFactor = (specularFactor + r) * 0.5;

	vec4 ambientColor = vec4(ambientLight.rgb * foam, 0.15);
	vec4 diffuseColor = vec4(diffuseLight.rgb * diffuseFactor * waterColor+noise*noise*0.5, 0.35);
	vec4 specColor = vec4(specularLight.rgb * specularFactor * diffuseFactor, fresnel_alpha);

	vec4 finalColor = vec4(0.0);
	finalColor.rgb = ambientColor.rgb + diffuseColor.rgb + specColor.rgb;
	finalColor.rgb = mix(finalColor.rgb, (finalColor.rgb+vec3(1.0,0.8,0.63))*0.5, fresnel);
	finalColor.a = (ambientColor.a + diffuseColor.a + specColor.a) * (1.0-depth);

	vec4 lightmap = texture(lightmap_tex, uvLightmap, 0.0);
	finalColor.rgb = blendAddEffectLighting(finalColor.rgb, (lightmap.rgb / 1.5f)); // additive color (from environmental point lights / effects)
	finalColor.rgb *= vec3(lightmap.a); // ... * tile brightness / ambient occlusion (stored in lightmap.a);

	return finalColor;
}

void main()
{
	vec4 fragColor = main_bumpMapping();

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - length(frag.posViewSpace)) / (fogEnd - fogStart);
		fragColor = mix(vec4(fragColor.rgb,fragColor.a), vec4(fogColor.rgb,fragColor.a), clamp(fogFactor, 0.0, 1.0));
	}

	FragColor = fragColor;
}
