#version 450

#include "terrain_combined.glsl"

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexNormal;
layout(location = 4) in vec4 vertexTangent;	// for decals
layout(location = 5) in int tileNo;
layout(location = 6) in uvec4 grounds;		// ground types for splatting
layout(location = 7) in vec4 groundWeights;	// ground weights for splatting

layout(location = 0) out FragData frag;
layout(location = 11) out flat FragFlatData fragf;
layout(location = 14) out mat3 ModelTangentMatrix;

void main()
{
	fragf.tileNo = tileNo;

	frag.uvLightmap = (ModelUVLightmapMatrix * vertex).xy;
	frag.uvGround = vec2(-vertex.z, vertex.x);
	frag.uvDecal = vertexTexCoord;

	fragf.grounds = grounds;
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
		ModelTangentMatrix = mat3(tangent, bitangent, vertexNormal); // aka TBN-matrix
		// transform light to TangentSpace:
		vec3 eyeVec = normalize((cameraPos.xyz - vertex.xyz) * ModelTangentMatrix);
		frag.groundLightDir = sunPos.xyz * ModelTangentMatrix; // already normalized
		frag.groundHalfVec = frag.groundLightDir + eyeVec;

		vec3 bitangentDecal = -cross(vertexNormal, vertexTangent.xyz) * vertexTangent.w;
		// transformation matrix from decal tangent space to ground tangent space for normals xy
		frag.decal2groundMat2 = mat2(
			dot(vertexTangent.xyz, tangent), dot(bitangentDecal, tangent),
			dot(vertexTangent.xyz, bitangent), dot(bitangentDecal, bitangent)
		);
	}

	frag.fragPos = vertex.xyz;
	frag.fragNormal = vertexNormal;

	vec4 position = ModelViewProjectionMatrix * vertex;
	frag.vertexDistance = position.z;
	gl_Position = position;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
