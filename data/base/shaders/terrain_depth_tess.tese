// Version directive is set by Warzone when loading the shader
// (Hardware-tessellated terrain - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

// Depth-prepass TES: evaluates the same baked surface as
// terrain_combined_tess.tese (positions must stay bit-identical), and
// produces the interface of terrain_depth.vert for terraindepth.frag.

#include "terrain_tess.glsl"

layout(quads, fractional_odd_spacing, ccw) in;

uniform mat4 ModelViewProjectionMatrix;

uniform vec4 paramx2;
uniform vec4 paramy2;

uniform mat4 textureMatrix2;

uniform sampler2D terrainBakedHeight;
uniform sampler2D terrainBakedOffset;
uniform sampler2D terrainBakedNormal;

out vec2 uv2;
out float vertexDistance;

void main()
{
	vec2 uv = gl_TessCoord.xy;
	vec2 latticeXY = wz_patchLatticeXY(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position, uv);
	vec3 posModel;
	vec3 unusedNormal;
	wz_evalTerrain(terrainBakedHeight, terrainBakedOffset, terrainBakedNormal, latticeXY, posModel, unusedNormal);
	vec4 vertex = vec4(posModel, 1.0);

	// ---- same computations as terrain_depth.vert ----
	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	vec4 uv2_tmp = textureMatrix2 * vec4(dot(paramx2, vertex), dot(paramy2, vertex), 0., 1.);
	uv2 = uv2_tmp.xy / uv2_tmp.w;
	vertexDistance = position.z;
}
