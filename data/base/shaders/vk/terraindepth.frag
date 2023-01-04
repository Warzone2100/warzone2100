#version 450

layout(set = 1, binding = 0) uniform sampler2D lightmap_tex;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	vec4 paramx1;
	vec4 paramy1;
	vec4 paramx2;
	vec4 paramy2;
	mat4 textureMatrix1;
	mat4 textureMatrix2;
	vec4 fogColor;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
};

layout(location = 2) in vec2 uv2;
layout(location = 3) in float vertexDistance;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 fragColor = texture(lightmap_tex, uv2);

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1)
		{
			discard;
		}
	}

	// Return fragment color
	FragColor = fragColor;
}
