#version 450
//#pragma debug(on)

#include "tcmask_instanced.glsl"

layout(set = 3, binding = 0) uniform sampler2D Texture;

layout(location = 0) in vec4 texCoord_vertexDistance; // vec(2) texCoord, float vertexDistance, (unused float)
layout(location = 1) in vec4 colour;
layout(location = 2) in vec4 packed_ecmState_alphaTest;
layout(location = 3) in vec3 posViewSpace;

layout(location = 0) out vec4 FragColor;

#include "distance_fog.glsl"

void main()
{
	// unpack inputs
	vec2 texCoord = vec2(texCoord_vertexDistance.x, texCoord_vertexDistance.y);
	bool alphaTest = (packed_ecmState_alphaTest.y > 0.f);

	vec4 texColour = texture(Texture, texCoord, WZ_MIP_LOAD_BIAS);

	vec4 fragColour = texColour * colour;
	if (fogRange.z > 0.5 && fogOutput != WZ_FOG_OUTPUT_DISABLED)
	{
		float fogAmount = wzDistanceFogAmount(length(posViewSpace), fogRange.x, fogRange.y);
		fragColour.rgb = wzApplyForwardFog(fragColour.rgb, fragColour.a, fogAmount, fogColor.rgb, fogOutput);
	}

	if (alphaTest && (fragColour.a <= 0.001))
	{
		discard;
	}

	FragColor = fragColour;
}
