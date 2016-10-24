#version 120

uniform mat4 ModelViewProjectionMatrix;

uniform vec4 paramx;
uniform vec4 paramy;
uniform vec4 paramxlight;
uniform vec4 paramylight;

varying vec4 color;
varying vec2 uv_tex;
varying vec2 uv_lightmap;

void main()
{
	color = gl_Color;
	gl_Position = ModelViewProjectionMatrix * gl_Vertex;
	uv_tex = vec2(dot(paramx, gl_Vertex), dot(paramy, gl_Vertex));
	uv_lightmap = vec2(dot(paramxlight, gl_Vertex), dot(paramylight, gl_Vertex));
}
