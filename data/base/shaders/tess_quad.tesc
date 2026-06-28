// Version directive is set by Warzone when loading the shader
// (Dev-only tessellation smoke test - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

layout(vertices = 4) out;

uniform float tessLevel;

void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelOuter[0] = tessLevel;
		gl_TessLevelOuter[1] = tessLevel;
		gl_TessLevelOuter[2] = tessLevel;
		gl_TessLevelOuter[3] = tessLevel;
		gl_TessLevelInner[0] = tessLevel;
		gl_TessLevelInner[1] = tessLevel;
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
