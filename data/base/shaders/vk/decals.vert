#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUVLightmapMatrix;
	vec4 cameraPos; // in modelSpace
	vec4 sunPos; // in modelSpace, normalized
	vec4 emissiveLight; // light colors/intensity
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
	vec4 fogColor;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	int quality;
};

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexNormal;
layout(location = 4) in vec4 vertexTangent;
layout(location = 5) in int tileNo;

layout(location = 0) out vec2 uv_tex;
layout(location = 1) out vec2 uv_lightmap;
layout(location = 2) out float vertexDistance;
layout(location = 3) out vec3 lightDir;
layout(location = 4) out vec3 halfVec;
//layout(location = 5) flat out int tile;

void main()
{
	//tile = tileNo;
	uv_tex = vertexTexCoord;
	uv_lightmap = (ModelUVLightmapMatrix * vertex).xy;

	// constructing ModelSpace -> TangentSpace mat3
	vec3 bitangent = cross(vertexNormal, vertexTangent.xyz) * vertexTangent.w;
	mat3 ModelTangentMatrix = mat3(vertexTangent.xyz, bitangent, vertexNormal);
	// transform light from ModelSpace to TangentSpace:
	vec3 eyeVec = normalize((cameraPos.xyz - vertex.xyz) * ModelTangentMatrix);
	lightDir = sunPos.xyz * ModelTangentMatrix;
	halfVec = lightDir + eyeVec;

	vec4 position = ModelViewProjectionMatrix * vertex;
	vertexDistance = position.z;
	gl_Position = position;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
