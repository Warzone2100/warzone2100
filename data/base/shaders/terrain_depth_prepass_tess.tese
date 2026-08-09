// Version directive is set by Warzone when loading the shader
// (Hardware-tessellated terrain - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

#include "terrain_tess.glsl"

layout(quads, fractional_odd_spacing, ccw) in;

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ViewMatrix;
uniform sampler2D terrainBakedHeight;
uniform sampler2D terrainBakedOffset;
uniform sampler2D terrainBakedNormal;

out vec3 viewNormal;

void main()
{
	vec2 latticeXY = wz_patchLatticeXY(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.xy);
	vec3 posModel;
	vec3 normalModel;
	wz_evalTerrain(terrainBakedHeight, terrainBakedOffset, terrainBakedNormal, latticeXY, posModel, normalModel);

	gl_Position = ModelViewProjectionMatrix * vec4(posModel, 1.0);
	viewNormal = mat3(ViewMatrix) * normalModel;
}
