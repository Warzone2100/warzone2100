#version 450

layout(location = 0) in vec3 viewNormal;
layout(location = 0) out vec4 FragColor;

#define MESH_SSAO_WEIGHT 1.0

void main()
{
	vec3 n = normalize(viewNormal);
	FragColor = vec4(n * 0.5 + 0.5, MESH_SSAO_WEIGHT);
}
