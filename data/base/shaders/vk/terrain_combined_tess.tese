#version 450

// Evaluates the baked terrain surface and produces exactly the interface of
// terrain_combined.vert, so the existing terrain fragment shaders (.frag.spv)
// are reused unchanged.

#include "terrain_combined.glsl"
#include "terrain_tess.glsl"

// cw, not ccw: this TES applies the Vulkan clip-space Y-flip, which inverts
// the screen-space winding of the tessellator-generated triangles (unlike
// normal draws, patch triangles get their winding from this declaration, not
// from vertex order). The pipeline declares clockwise front faces to match
// Y-flipped vertex-buffer geometry, so the domain winding must be cw here.
layout(quads, fractional_odd_spacing, cw) in;

layout(set = 2, binding = 10) uniform sampler2D terrainBakedHeight;
layout(set = 2, binding = 11) uniform sampler2D terrainBakedOffset;
layout(set = 2, binding = 12) uniform sampler2D terrainBakedNormal;

layout(location = 0) in vec2 teTexCoord[];
layout(location = 1) in vec4 teTangent[];
layout(location = 2) in flat int teTileNo[];
layout(location = 3) in flat uvec4 teGrounds[];
layout(location = 4) in vec4 teGroundWeights[];

// must match terrain_combined.vert's output interface (consumed by the terrain fragment shaders)
layout(location = 0) out FragData frag;
layout(location = 10) out flat FragFlatData fragf;
layout(location = 12) out mat3 ModelTangentMatrix;

void main()
{
	vec2 uv = gl_TessCoord.xy;

	// evaluate the baked surface at this lattice point
	vec2 latticeXY = wz_patchLatticeXY(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position, uv);
	vec3 posModel;
	vec3 vertexNormal;
	wz_evalTerrain(terrainBakedHeight, terrainBakedOffset, terrainBakedNormal, latticeXY, posModel, vertexNormal);
	vec4 vertex = vec4(posModel, 1.0);

	// interpolate the per-corner attributes (matches the CPU subdivided builder:
	// bilinear decal UVs and bilinear one-hot ground weights)
	vec2 vertexTexCoord = WZ_PATCH_BILINEAR(teTexCoord, uv.x, uv.y);
	vec4 tangentRaw = WZ_PATCH_BILINEAR(teTangent, uv.x, uv.y);
	vec4 vertexTangent = vec4(normalize(tangentRaw.xyz), teTangent[0].w);
	vec4 groundWeights = WZ_PATCH_BILINEAR(teGroundWeights, uv.x, uv.y);

	// from here on: same computations as terrain_combined.vert
	fragf.tileNo = teTileNo[0];

	frag.uvLightmap = (ModelUVLightmapMatrix * vertex).xy;
	frag.uvGround = vec2(-vertex.z, vertex.x);
	frag.uvDecal = vertexTexCoord;

	fragf.grounds = teGrounds[0];
	if (groundWeights == vec4(0)) {
		frag.groundWeights = vec4(0.25);
	} else {
		frag.groundWeights = groundWeights;
	}

	{ // calc light
		// constructing ModelSpace -> TangentSpace mat3
		vec3 vaxis = vec3(1,0,0); // v ~ vertex.x, see uv_ground
		vec3 tangent = normalize(cross(vertexNormal, vaxis));
		vec3 bitangent = cross(vertexNormal, tangent);
		//transpose tbn matrix to fit universal point lights shader
		ModelTangentMatrix = transpose(mat3(tangent, bitangent, vertexNormal)); // aka TBN-matrix
		// transform light to TangentSpace:
		vec3 eyeVec = ModelTangentMatrix * normalize(cameraPos.xyz - vertex.xyz);
		frag.groundLightDir = ModelTangentMatrix * sunPos.xyz; // already normalized
		frag.groundHalfVec = frag.groundLightDir + eyeVec;

		vec3 bitangentDecal = cross(vertexNormal, vertexTangent.xyz) * vertexTangent.w;
		// transformation matrix from decal tangent space to ground tangent space for normals xy
		frag.decal2groundMat2 = mat2(
			dot(vertexTangent.xyz, tangent), dot(bitangentDecal, tangent),
			dot(vertexTangent.xyz, bitangent), dot(bitangentDecal, bitangent)
		);
	}

	frag.posModelSpace = vertex.xyz;
	frag.posViewSpace = (ViewMatrix * vec4(vertex.xyz,1.0)).xyz;

	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	// Vulkan clip-space fixup (this is the final position-producing stage)
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
