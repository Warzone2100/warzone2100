#version 120

uniform mat4 posMatrix;

attribute vec4 vertex;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;

varying vec2 uv;
varying vec4 color;

void main()
{
	// Pass texture coordinates to fragment shader
	uv = vertexTexCoord;

	gl_Position = posMatrix * vertex;
	color = vertexColor;
}
