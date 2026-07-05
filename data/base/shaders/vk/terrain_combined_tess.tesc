#version 450

#include "terrain_combined.glsl"
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
		float outerLevels[4];
		float innerLevels[2];
		// the color/depth-prepass passes ARE the main camera, so the pass and factor transforms coincide here
		wz_terrainTessLevels(ModelViewProjectionMatrix, ModelViewProjectionMatrix, float(viewportHeight), tessMaxLevel,
							 gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz, gl_in[3].gl_Position.xyz,
							 outerLevels, innerLevels);
		gl_TessLevelOuter[0] = outerLevels[0];
		gl_TessLevelOuter[1] = outerLevels[1];
		gl_TessLevelOuter[2] = outerLevels[2];
		gl_TessLevelOuter[3] = outerLevels[3];
		gl_TessLevelInner[0] = innerLevels[0];
		gl_TessLevelInner[1] = innerLevels[1];
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	teTexCoord[gl_InvocationID] = tcTexCoord[gl_InvocationID];
	teTangent[gl_InvocationID] = tcTangent[gl_InvocationID];
	teTileNo[gl_InvocationID] = tcTileNo[gl_InvocationID];
	teGrounds[gl_InvocationID] = tcGrounds[gl_InvocationID];
	teGroundWeights[gl_InvocationID] = tcGroundWeights[gl_InvocationID];
}
