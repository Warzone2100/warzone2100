// Version directive is set by Warzone when loading the shader
// (Hardware-tessellated terrain - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

// Passthrough of the tile-corner control points to the tessellation stages.
// All real work happens in terrain_combined_tess.tese.

#include "terrain_combined.glsl"

in vec4 vertex;
in vec2 vertexTexCoord;
in vec4 vertexTangent;
in int tileNo;
in uvec4 grounds;
in vec4 groundWeights;

out vec2 tcTexCoord;
out vec4 tcTangent;
out int tcTileNo;
out uvec4 tcGrounds;
out vec4 tcGroundWeights;

void main()
{
	gl_Position = vertex;
	tcTexCoord = vertexTexCoord;
	tcTangent = vertexTangent;
	tcTileNo = tileNo;
	tcGrounds = grounds;
	tcGroundWeights = groundWeights;
}
