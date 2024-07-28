#version 450
//#pragma debug(on)

#include "tcmask_instanced.glsl"

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;

layout(set = 2, binding = 0) uniform sampler2D Texture;

layout(location = 0) in vec4 texCoord_vertexDistance; // vec(2) texCoord, float vertexDistance, (unused float)
layout(location = 1) in vec4 colour;
layout(location = 2) in vec4 packed_ecmState_alphaTest;

layout(location = 0) out vec4 FragColor;

void main()
{
	// unpack inputs
	vec2 texCoord = vec2(texCoord_vertexDistance.x, texCoord_vertexDistance.y);
	float vertexDistance = texCoord_vertexDistance.z;
	bool alphaTest = (packed_ecmState_alphaTest.y > 0.f);

	vec4 texColour = texture(Texture, texCoord, WZ_MIP_LOAD_BIAS);

	vec4 fragColour = texColour * colour;

	if (alphaTest && (fragColour.a <= 0.001))
	{
		discard;
	}

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1.f)
		{
			discard;
		}

		// Return fragment color
		vec3 fogPremultAlphaFactor = mix(vec3(fragColour.a), vec3(1.f,1.f,1.f), vec3(float(alphaTest)));
		float fogFactorAdjust = mix(1.f, 0.f, float(alphaTest));
		fragColour = vec4(mix(fragColour.rgb, fogColor.rgb * fogPremultAlphaFactor, clamp(fogFactor * fogFactorAdjust, 0.0, 1.0)), fragColour.a);
		fragColour.a = fragColour.a * (1.0 - clamp(fogFactor, 0.0, 1.0));
	}

	FragColor = fragColour;
}
