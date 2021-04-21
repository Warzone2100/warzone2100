#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer
{
	mat4 posMatrix;
	vec4 color;
	vec4 fog_color;
	int fog_enabled;
};

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 vertexTexCoord;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 fog;

void main()
{
	// Pass texture coordinates to fragment shader
	uv = vertexTexCoord;

	gl_Position = posMatrix * vertex;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;

	if(fog_enabled > 0)
	{
		fog =
		vertex.y < -0.7 ? vec4(0, 0, 0, 1) :
		vertex.y < 0.5 ? fog_color :
		vec4(fog_color.xyz, 0);
	}
}
