#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer
{
	mat4 transform_matrix;
	vec4 color;
	vec4 fog_color;
	int fog_enabled;
	int linearLighting;
};
layout(set = 1, binding = 0) uniform sampler2D theTexture;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 fog;

layout(location = 0) out vec4 FragColor;

#include "srgb.glsl"

void main()
{
	vec4 resultingColor = texture(theTexture, uv) * color;

	if (linearLighting != 0)
	{
		// The sky is drawn first, over cleared black, so its alpha fade reduces to the color times the alpha.
		// This shader applies the fade, tint and fog in display space and writes an opaque result.
		vec3 displayColor = wzLinearToSrgb(resultingColor.rgb);
		float alpha = resultingColor.a;
		if(fog_enabled > 0)
		{
			displayColor = mix(displayColor, fog.xyz, fog.w);
			alpha = mix(alpha, 1.0, fog.w);
		}
		resultingColor = vec4(wzSrgbToLinear(displayColor * alpha), 1.0);
	}
	else if(fog_enabled > 0)
	{
		resultingColor = mix(resultingColor, vec4(fog.xyz, 1), fog.w);
	}

	FragColor = resultingColor;
}
