#version 450

#include "terrain_tess.glsl"

layout(vertices = 4) out;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ViewMatrix;
	mat4 tessCameraMVP;
	vec4 tessParams;
};

void main()
{
	if (gl_InvocationID == 0)
	{
		float outerLevels[4];
		float innerLevels[2];
		wz_terrainTessLevels(ModelViewProjectionMatrix, tessCameraMVP, tessParams.y, tessParams.x,
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
}
