// Version directive is set by Warzone when loading the shader
// (Hardware-tessellated terrain - requires GLSL 4.00 core, or 3.30 core + GL_ARB_tessellation_shader)

// Evaluates the baked terrain surface and produces exactly the interface of
// terrain_combined.vert, so the existing terrain fragment shaders are reused
// unchanged.

#include "terrain_tess.glsl"

layout(quads, equal_spacing, ccw) in;

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelUVLightmapMatrix;
uniform mat4 ViewMatrix;

uniform vec4 cameraPos; // in modelSpace
uniform vec4 sunPos; // in modelSpace, normalized

uniform sampler2D terrainBakedHeight;
uniform sampler2D terrainBakedOffset;
uniform sampler2D terrainBakedNormal;

in vec2 teTexCoord[];
in vec4 teTangent[];
in int teTileNo[];
in uvec4 teGrounds[];
in vec4 teGroundWeights[];

// must match terrain_combined.vert's output interface (consumed by the terrain fragment shaders)
out vec2 uvLightmap;
out vec2 uvGround;
out vec2 uvDecal;
flat out int tile;
flat out uvec4 fgrounds;
out vec4 fgroundWeights;
// In tangent space
out vec3 groundLightDir;
out vec3 groundHalfVec;
out mat2 decal2groundMat2;

out mat3 ModelTangentMatrix;
// for Shadows
out vec3 posModelSpace;
out vec3 posViewSpace;

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
	tile = teTileNo[0];

	uvLightmap = (ModelUVLightmapMatrix * vertex).xy;
	uvGround = vec2(-vertex.z, vertex.x);
	uvDecal = vertexTexCoord;

	fgrounds = teGrounds[0];
	fgroundWeights = groundWeights;
	if (fgroundWeights == vec4(0)) {
		fgroundWeights = vec4(0.25);
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
		groundLightDir = ModelTangentMatrix * sunPos.xyz; // already normalized
		groundHalfVec = groundLightDir + eyeVec;

		vec3 bitangentDecal = -cross(vertexNormal, vertexTangent.xyz) * vertexTangent.w;
		// transformation matrix from decal tangent space to ground tangent space for normals xy
		decal2groundMat2 = mat2(
			dot(vertexTangent.xyz, tangent), dot(bitangentDecal, tangent),
			dot(vertexTangent.xyz, bitangent), dot(bitangentDecal, bitangent)
		);
	}

	posModelSpace = vertex.xyz;
	posViewSpace = (ViewMatrix * vec4(vertex.xyz,1.0)).xyz;

	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
}
