#version 130

uniform vec4 color = vec4(1.);
uniform sampler2D texture;

in vec2 uv;
out vec4 FragColor;

void main()
{
	FragColor = texture2D(texture, uv) * color;
}
