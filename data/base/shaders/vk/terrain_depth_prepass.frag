#version 450

layout(location = 0) in vec3 viewNormal;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec3 n = normalize(viewNormal);
	// Alpha = SSAO application weight (1.0 = full terrain AO).
	FragColor = vec4(n * 0.5 + 0.5, 1.0);
}
