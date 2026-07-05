#version 450

// Depth-prepass TES: evaluates the same baked surface as
// terrain_combined_tess.tese (positions must stay bit-identical), and
// produces the interface of terrain_depth.vert for terraindepth.frag.

#include "terrain_tess.glsl"

// cw, not ccw: this TES applies the Vulkan clip-space Y-flip, which inverts
// the screen-space winding of the tessellator-generated triangles (unlike
// normal draws, patch triangles get their winding from this declaration, not
// from vertex order). The pipeline declares clockwise front faces to match
// Y-flipped vertex-buffer geometry, so the domain winding must be cw here.
layout(quads, fractional_odd_spacing, cw) in;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 tessCameraMVP;
	vec4 paramx2;
	vec4 paramy2;
	mat4 textureMatrix2;
	vec4 tessParams;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
};

layout(set = 1, binding = 1) uniform sampler2D terrainBakedHeight;
layout(set = 1, binding = 2) uniform sampler2D terrainBakedOffset;
layout(set = 1, binding = 3) uniform sampler2D terrainBakedNormal;

layout(location = 2) out vec2 uv2;
layout(location = 3) out float vertexDistance;

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
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
