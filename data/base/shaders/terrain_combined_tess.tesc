// Version directive is set by Warzone when loading the shader
// (Hardware-tessellated terrain - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

#include "terrain_combined.glsl"

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
		float outerLevels[4];
		float innerLevels[2];
		// the color pass IS the main camera, so the pass and factor transforms coincide here
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
