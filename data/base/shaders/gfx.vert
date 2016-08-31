#version 130

uniform mat4 posMatrix;

in vec4 vertex;
in vec4 vertexTexCoord;
in vec4 vertexColor;

out vec2 uv;
out vec4 color;

void main()
{
	// Pass texture coordinates to fragment shader
	uv = vertexTexCoord.xy;

	gl_Position = posMatrix * vertex;
	color = vertexColor;
}
