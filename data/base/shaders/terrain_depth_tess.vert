// Version directive is set by Warzone when loading the shader
// (Hardware-tessellated terrain - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

// Passthrough of the tile-corner control points (positions only) for the
// terrain depth passes.

in vec4 vertex;

void main()
{
	gl_Position = vertex;
}
