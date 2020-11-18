#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer
{
	mat4 posMatrix;
	vec2 offset;
	vec2 size;
	vec4 color;
	int theTexture;
	int fog_enabled;
	vec4 fog_color;
};

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec4 vertexColor;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 vColour;
layout(location = 2) out vec4 fog;

void main()
{
	// Pass texture coordinates to fragment shader
	uv = vertexTexCoord;

	gl_Position = posMatrix * vertex;

	vColour = vertexColor;
	if(fog_enabled > 0)
	{
		fog =
		vertex.y < -0.5 ? vec4(0, 0, 0, 1) :
		vertex.y < 0.5 ? fog_color :
		vec4(fog_color.xyz, 0);
	}
}
