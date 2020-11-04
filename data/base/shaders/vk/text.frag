#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	vec2 tuv_offset;
	vec2 tuv_scale;
	vec4 color;
};

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 texColour = texture(tex, uv) * color.a;
	FragColor = texColour * color;

	// gl_FragData[1] apparently fails to compile for some people, see #4584.
	// GL::SC(Error:High) : 0:12(2): error: array index must be < 1
	//gl_FragData[0] = texColour * color;
	//gl_FragData[1] = texColour;
}
