#version 450

layout(location = 0) in vec3 viewNormal;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec3 n = normalize(viewNormal);
	// Alpha = SSAO application weight. Water is 0 so SSAO skips lakes and SSR can identify them.
	FragColor = vec4(n * 0.5 + 0.5, 0.0);
}
