#version 450

#include "terrain_tess.glsl"

layout(quads, fractional_odd_spacing, ccw) in;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ViewMatrix;
	mat4 tessCameraMVP;
	vec4 tessParams;
};

layout(set = 1, binding = 0) uniform sampler2D terrainBakedHeight;
layout(set = 1, binding = 1) uniform sampler2D terrainBakedOffset;
layout(set = 1, binding = 2) uniform sampler2D terrainBakedNormal;

layout(location = 0) out vec3 viewNormal;

void main()
{
	vec2 latticeXY = wz_patchLatticeXY(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.xy);
	vec3 posModel;
	vec3 normalModel;
	wz_evalTerrain(terrainBakedHeight, terrainBakedOffset, terrainBakedNormal, latticeXY, posModel, normalModel);

	gl_Position = ModelViewProjectionMatrix * vec4(posModel, 1.0);
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	viewNormal = mat3(ViewMatrix) * normalModel;
}
