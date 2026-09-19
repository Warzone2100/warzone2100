// Version directive is set by Warzone when loading the shader
// (Dev-only tessellation smoke test - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

in vec2 vertex;

void main()
{
	// control points pass through to the tessellation stages. The transform happens in the TES.
	gl_Position = vec4(vertex, 0.0, 1.0);
}
