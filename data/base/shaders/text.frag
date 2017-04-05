#version 130

uniform vec4 color = vec4(1.);
uniform sampler2D texture;

in vec2 uv;
out vec4 FragColor;
out vec4 FragColor1;

void main()
{
	vec4 texColour = texture2D(texture, uv) * color.a;
	FragColor = texColour * color;
	FragColor1 = texColour;
}
