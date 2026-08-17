#version 450

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec3 centerRadius;

layout(location = 0) out vec4 FragColor;

void main()
{
	float R = max(centerRadius.z, 1e-3);
	float d = length(worldPos.xz - centerRadius.xy);
	float sdf = d - R;
	if (sdf > 0.25 * R)
	{
		discard;
	}
	float encoded = 0.5 + 0.5 * clamp(sdf / R, -1.0, 1.0);
	FragColor = vec4(encoded, encoded, encoded, 1.0);
}
