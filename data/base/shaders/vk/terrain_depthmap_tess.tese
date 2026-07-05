#version 450

// Shadow/depth-map TES: evaluates the same baked surface as
// terrain_combined_tess.tese (positions must stay bit-identical), and
// mirrors terrain_depth_only.vert.

#include "terrain_tess.glsl"

layout(quads, equal_spacing, ccw) in;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
};

layout(set = 1, binding = 1) uniform sampler2D terrainBakedHeight;
layout(set = 1, binding = 2) uniform sampler2D terrainBakedOffset;
layout(set = 1, binding = 3) uniform sampler2D terrainBakedNormal;

void main()
{
	vec2 uv = gl_TessCoord.xy;
	vec2 latticeXY = wz_patchLatticeXY(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position, uv);
	vec3 posModel;
	vec3 unusedNormal;
	wz_evalTerrain(terrainBakedHeight, terrainBakedOffset, terrainBakedNormal, latticeXY, posModel, unusedNormal);

	// same computations as terrain_depth_only.vert
	vec4 position = ModelViewProjectionMatrix * vec4(posModel, 1.0);
	position.z += 0.01f;
	gl_Position = position;

//	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
