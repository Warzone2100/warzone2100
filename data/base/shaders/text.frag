#version 120

uniform vec4 color;
uniform sampler2D texture;

varying vec2 uv;

void main()
{
	vec4 texColour = texture2D(texture, uv) * color.a;
	gl_FragData[0] = texColour * color;
	gl_FragData[1] = texColour;
}
