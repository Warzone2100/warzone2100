#version 120

uniform mat4 ModelViewProjectionMatrix;

uniform vec4 paramx1;
uniform vec4 paramy1;
uniform vec4 paramx2;
uniform vec4 paramy2;

uniform mat4 textureMatrix1 = mat4(1.);
uniform mat4 textureMatrix2 = mat4(1.);

attribute vec4 vertex;
attribute vec4 vertexColor;

varying vec4 color;
varying vec2 uv1;
varying vec2 uv2;
varying float vertexDistance;

void main()
{
	color = vertexColor;
	gl_Position = ModelViewProjectionMatrix * vertex;
	vec4 uv1_tmp = textureMatrix1 * vec4(dot(paramx1, vertex), dot(paramy1, vertex), 0., 1.);
	uv1 = uv1_tmp.xy / uv1_tmp.w;
	vec4 uv2_tmp = textureMatrix2 * vec4(dot(paramx2, vertex), dot(paramy2, vertex), 0., 1.);
	uv2 = uv2_tmp.xy / uv2_tmp.w;
	vertexDistance = gl_Position.z;
}
