#version 450

layout(set = 1, binding = 0) uniform sampler2D tex;
layout(set = 1, binding = 1) uniform sampler2D lightmap_tex;

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

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv1;
layout(location = 2) in vec2 uv2;
layout(location = 3) in float vertexDistance;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 fragColor = color * texture(tex, uv1) * texture(lightmap_tex, uv2);
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fogColor, fragColor, fogFactor);
	}
	FragColor = fragColor;
}
