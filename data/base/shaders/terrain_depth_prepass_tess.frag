// Version directive is set by Warzone when loading the shader
// (Hardware-tessellated terrain - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

in vec3 viewNormal;
out vec4 FragColor;

void main()
{
	vec3 n = normalize(viewNormal);
	// Alpha = SSAO application weight (1.0 = full terrain AO).
	FragColor = vec4(n * 0.5 + 0.5, 1.0);
}
