#version 120

uniform mat4 ModelViewProjectionMatrix;

attribute vec4 vertex;

void main()
{
	gl_Position = ModelViewProjectionMatrix * vertex;
}