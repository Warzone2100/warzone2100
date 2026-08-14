#version 450
//#pragma debug(on)

#include "tcmask_instanced.glsl"

layout(set = 3, binding = 0) uniform sampler2D Texture;

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

	FragColor = fragColour;
}
