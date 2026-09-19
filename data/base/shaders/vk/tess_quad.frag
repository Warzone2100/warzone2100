#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	vec4 color;
	float tessLevel;
};

layout(location = 0) in vec2 tessUV;

layout(location = 0) out vec4 FragColor;

void main()
{
	// visualize the tessellation grid: dark lines along sub-patch boundaries
	vec2 cells = tessUV * max(tessLevel, 1.0);
	vec2 distToLine = abs(fract(cells) - 0.5);
	float line = smoothstep(0.35, 0.5, max(distToLine.x, distToLine.y));
	vec3 base = mix(color.rgb, vec3(tessUV, 1.0 - tessUV.x), 0.5);
	FragColor = vec4(mix(base, vec3(0.0), line * 0.85), color.a);
}
