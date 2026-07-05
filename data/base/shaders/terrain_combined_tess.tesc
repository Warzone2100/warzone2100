// Version directive is set by Warzone when loading the shader
// (Hardware-tessellated terrain - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

#include "terrain_tess.glsl"

layout(vertices = 4) out;

in vec2 tcTexCoord[];
in vec4 tcTangent[];
in int tcTileNo[];
in uvec4 tcGrounds[];
in vec4 tcGroundWeights[];

out vec2 teTexCoord[];
out vec4 teTangent[];
out int teTileNo[];
out uvec4 teGrounds[];
out vec4 teGroundWeights[];

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
	teTexCoord[gl_InvocationID] = tcTexCoord[gl_InvocationID];
	teTangent[gl_InvocationID] = tcTangent[gl_InvocationID];
	teTileNo[gl_InvocationID] = tcTileNo[gl_InvocationID];
	teGrounds[gl_InvocationID] = tcGrounds[gl_InvocationID];
	teGroundWeights[gl_InvocationID] = tcGroundWeights[gl_InvocationID];
}
