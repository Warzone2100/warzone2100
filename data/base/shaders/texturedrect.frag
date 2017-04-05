#version 120

uniform vec4 color = vec4(1.);
uniform sampler2D texture;

varying vec2 uv;

void main()
{
	gl_FragColor = texture2D(texture, uv) * color;
}
