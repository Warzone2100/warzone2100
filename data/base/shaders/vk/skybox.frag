#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer
{
	mat4 transform_matrix;
	vec2 offset;
	vec2 size;
	vec4 color;
	int theTexture;
	int fog_enabled;
	vec4 fog_color;
};

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 fog;

layout(location = 0) out vec4 FragColor;

void main()
{
	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	vec4 resultingColor = texture(theTexture, uv) * color;
	#else
	vec4 resultingColor = texture2D(theTexture, uv) * color;
	#endif

	if(fog_enabled > 0)
	{
		resultingColor = mix(resultingColor, vec4(fog.xyz, 1), fog.w);
	}

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = resultingColor;
	#else
	gl_FragColor = resultingColor;
	#endif
}
