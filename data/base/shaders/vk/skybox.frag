#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer
{
	mat4 transform_matrix;
	vec4 color;
	vec4 fog_color;
	int fog_enabled;
};
layout(set = 1, binding = 0) uniform sampler2D theTexture;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 fog;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 resultingColor = texture(theTexture, uv) * color;

	if(fog_enabled > 0)
	{
		resultingColor = mix(resultingColor, vec4(fog.xyz, 1), fog.w);
	}

	FragColor = resultingColor;
}
