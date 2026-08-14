// Version directive is set by Warzone when loading the shader
// (Dev-only tessellation smoke test - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

layout(std140) uniform cbuffer {
	mat4 transformationMatrix;
	vec4 color;
	float tessLevel;
};

in vec2 tessUV;

out vec4 FragColor;

void main()
{
	// visualize the tessellation grid: dark lines along sub-patch boundaries
	vec2 cells = tessUV * max(tessLevel, 1.0);
	vec2 distToLine = abs(fract(cells) - 0.5);
	float line = smoothstep(0.35, 0.5, max(distToLine.x, distToLine.y));
	vec3 base = mix(color.rgb, vec3(tessUV, 1.0 - tessUV.x), 0.5);
	FragColor = vec4(mix(base, vec3(0.0), line * 0.85), color.a);
}
