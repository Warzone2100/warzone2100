#version 120

uniform vec4 paramx;
uniform vec4 paramy;
uniform vec4 paramx2;
uniform vec4 paramy2;

varying vec4 color;
varying vec2 uv1;
varying vec2 uv2;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	uv1 = vec2(dot(paramx, gl_Vertex), dot(paramy, gl_Vertex));
	uv2 = vec2(dot(paramx2, gl_Vertex), dot(paramy2, gl_Vertex));
}
