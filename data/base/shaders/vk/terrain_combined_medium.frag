#version 450

#include "terrain_combined.glsl"

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;
layout (constant_id = 1) const uint WZ_EXTRA_SHADOW_TAPS = 4;

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


float getShadowVisibility()
{
	vec4 pos = frag.shadowPos / frag.shadowPos.w;
	if (pos.z > 1.0f)
	{
		return 1.0;
	}

	float bias = 0.0001f;

	float visibility = texture( shadowMap, vec3(pos.xy, (pos.z+bias)) );

	// PCF
	const float edgeVal = 0.5+float((WZ_EXTRA_SHADOW_TAPS-1)/2);
	const float startVal = -edgeVal;
	const float endVal = edgeVal + 0.5;
	const float texelIncrement = 1.0/float(4096);
	const float visibilityIncrement = 0.1; //0.5 / WZ_EXTRA_SHADOW_TAPS;
	for (float y=startVal; y<endVal; y+=1.0)
	{
		for (float x=startVal; x<endVal; x+=1.0)
		{
			visibility -= visibilityIncrement*(1.0-texture( shadowMap, vec3(pos.xy + vec2(x*texelIncrement, y*texelIncrement), (pos.z+bias)) ));
		}
	}

	visibility = clamp(visibility, 0.3, 1.0);

	return visibility;
}

vec3 getGroundUv(int i) {
	uint groundNo = fragf.grounds[i];
	return vec3(frag.uvGround * groundScale[groundNo/4u][groundNo%4u], groundNo);
}

vec3 getGround(int i) {
	return texture(groundTex, getGroundUv(i), WZ_MIP_LOAD_BIAS).rgb * frag.groundWeights[i];
}

vec3 blendAddEffectLighting(vec3 a, vec3 b) {
	return min(a + b, vec3(1.0));
}

vec4 main_medium() {
	vec3 ground = getGround(0) + getGround(1) + getGround(2) + getGround(3);
	vec4 decal = fragf.tileNo >= 0 ? texture(decalTex, vec3(frag.uvDecal, fragf.tileNo), WZ_MIP_LOAD_BIAS) : vec4(0.f);
	float visibility = getShadowVisibility();

	vec3 L = normalize(frag.groundLightDir);
	vec3 N = vec3(0.f,0.f,1.f);
	float lambertTerm = max(dot(N, L), 0.0); // diffuse lighting
	vec4 lightmap_vec4 = texture(lightmap_tex, frag.uvLightmap);
	vec4 light = (visibility*diffuseLight*0.75*lambertTerm + ambientLight*0.25) * lightmap_vec4.a; // ... * tile brightness / ambient occlusion (stored in lightmap.a)
	light.rgb = blendAddEffectLighting(light.rgb, (lightmap_vec4.rgb / 1.5f)); // additive color (from environmental point lights / effects)
	light.a = 1.f;

	return light * vec4((1.f - decal.a) * ground + decal.a * decal.rgb, 1.f);
}

void main()
{
	vec4 fragColor = main_medium();

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
