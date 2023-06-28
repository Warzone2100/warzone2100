#version 450

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;

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
	vec4 fragColor = color * texture(tex, uv1, WZ_MIP_LOAD_BIAS) * vec4(texture(lightmap_tex, uv2, 0.f).rgb, 1.f);
	
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1.0)
		{
			discard;
		}

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), clamp(fogFactor, 0.0, 1.0));
	}
	
	FragColor = fragColor;
}
