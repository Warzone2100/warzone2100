#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	vec2 tuv_offset;
	vec2 tuv_scale;
	vec4 color;
};

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec4 vertexColor;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 vColour;

void main()
{
	// Pass texture coordinates to fragment shader
	uv = vertexTexCoord;

	vColour = vertexColor;

	gl_Position = transformationMatrix * vertex;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
