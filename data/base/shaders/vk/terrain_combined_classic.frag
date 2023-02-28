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

layout(location = 0) in FragData frag;
layout(location = 9) flat in FragFlatData fragf;

layout(location = 0) out vec4 FragColor;

vec4 main_classic() {
	vec4 decal = fragf.tileNo >= 0 ? texture(decalTex, vec3(frag.uvDecal, fragf.tileNo), WZ_MIP_LOAD_BIAS) : vec4(0);
	vec4 light = texture(lightmap_tex, frag.uvLightmap, 0.f);
	return light * decal;
}

void main()
{
	vec4 fragColor = main_classic();

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
