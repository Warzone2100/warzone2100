#version 450

#include "terrain_tess.glsl"

layout(vertices = 4) out;

layout(location = 0) in vec2 tcTexCoord[];
layout(location = 1) in vec4 tcTangent[];
layout(location = 2) in flat int tcTileNo[];
layout(location = 3) in flat uvec4 tcGrounds[];
layout(location = 4) in vec4 tcGroundWeights[];

layout(location = 0) out vec2 teTexCoord[];
layout(location = 1) out vec4 teTangent[];
layout(location = 2) out flat int teTileNo[];
layout(location = 3) out flat uvec4 teGrounds[];
layout(location = 4) out vec4 teGroundWeights[];

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
