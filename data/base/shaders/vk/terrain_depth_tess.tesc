#version 450

#include "terrain_tess.glsl"

layout(vertices = 4) out;

void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelOuter[0] = WZ_TESS_LEVEL;
		gl_TessLevelOuter[1] = WZ_TESS_LEVEL;
		gl_TessLevelOuter[2] = WZ_TESS_LEVEL;
		gl_TessLevelOuter[3] = WZ_TESS_LEVEL;
		gl_TessLevelInner[0] = WZ_TESS_LEVEL;
		gl_TessLevelInner[1] = WZ_TESS_LEVEL;
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
