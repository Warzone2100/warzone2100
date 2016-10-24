#version 120

uniform sampler2D lightmap_tex;
varying vec2 uv2;

void main()
{
	gl_FragColor = texture2D(lightmap_tex, uv2);
}
