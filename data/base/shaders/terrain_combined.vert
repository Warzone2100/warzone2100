// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.30 - 1.50 core.)

// Aspects of shader limiting GLSL compat:
// - "flat" interpolation_qualifier (Desktop GLSL 130+, or GLES 300+)
#if (!defined(GL_ES) && (__VERSION__ < 130)) || (defined(GL_ES) && (__VERSION__ < 300))
#error "Unsupported version of GLSL"
#endif

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelUVLightmapMatrix;

uniform vec4 cameraPos; // in modelSpace
uniform vec4 sunPos; // in modelSpace, normalized

in vec4 vertex;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexTangent; // only for decals
in int tileNo; // positive if decal, negative for old tiles
in uvec4 grounds; // ground types for splatting
in vec4 groundWeights; // ground weights for splatting

out vec2 uvLightmap;
out vec2 uvGround;
out vec2 uvDecal;
out float vertexDistance;
flat out int tile;
flat out uvec4 fgrounds;
out vec4 fgroundWeights;
// In tangent space
out vec3 groundLightDir;
out vec3 groundHalfVec;
out mat2 decal2groundMat2;

out mat3 ModelTangentMatrix;
// for Shadows
out vec3 fragPos;
out vec3 fragNormal;

void main()
{
	tile = tileNo;

	uvLightmap = (ModelUVLightmapMatrix * vertex).xy;
	uvGround = vec2(-vertex.z, vertex.x);
	uvDecal = vertexTexCoord;

	fgrounds = grounds;
	fgroundWeights = groundWeights;
	if (fgroundWeights == vec4(0)) {
		fgroundWeights = vec4(0.25);
	}

	{ // calc light
		// constructing ModelSpace -> TangentSpace mat3
		vec3 vaxis = vec3(1,0,0); // v ~ vertex.x, see uv_ground
		vec3 tangent = normalize(cross(vertexNormal, vaxis));
		vec3 bitangent = cross(vertexNormal, tangent);
		ModelTangentMatrix = mat3(tangent, bitangent, vertexNormal); // aka TBN-matrix
		// transform light to TangentSpace:
		vec3 eyeVec = normalize((cameraPos.xyz - vertex.xyz) * ModelTangentMatrix);
		groundLightDir = sunPos.xyz * ModelTangentMatrix; // already normalized
		groundHalfVec = groundLightDir + eyeVec;

		vec3 bitangentDecal = -cross(vertexNormal, vertexTangent.xyz) * vertexTangent.w;
		// transformation matrix from decal tangent space to ground tangent space for normals xy
		decal2groundMat2 = mat2(
			dot(vertexTangent.xyz, tangent), dot(bitangentDecal, tangent),
			dot(vertexTangent.xyz, bitangent), dot(bitangentDecal, bitangent)
		);
	}

	fragPos = vertex.xyz;
	fragNormal = vertexNormal;

	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	vertexDistance = position.z;
}
