#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelUVMatrix;
	mat4 ModelUVLightMatrix;
	mat4 ModelViewMatrix;
	mat4 ModelViewProjectionMatrix;
	mat4 ModelViewNormalMatrix;
	vec4 sunPosition; // In EyeSpace
	vec4 emissiveLight; // light colors/intensity
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
	vec4 fogColor;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	int texture0;
	int texture1;
	int hasNormalmap;
	int hasSpecularmap;
};

layout(location = 0) in vec4 vertex;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec3 vertexNormal;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec2 uvLight;
layout(location = 3) out float vertexDistance;
// In tangent space
layout(location = 4) out vec3 lightDir;
layout(location = 5) out vec3 halfVec;

void main()
{
	color = vertexColor;

	uv = (ModelUVMatrix * vertex).xy;
	uvLight = (ModelUVLightMatrix * vertex).xy;

	// constructing EyeSpace -> TangentSpace mat3
	mat3 normalMat = mat3(ModelViewNormalMatrix);
	vec3 normal = normalize(normalMat * vertexNormal);
	vec3 vaxis = transpose(ModelUVMatrix)[1].xyz;
	vec3 tangent = normalize(cross(normal, normalMat * vaxis));
	vec3 bitangent = cross(normal, tangent);
	mat3 eyeTangentMatrix = mat3(tangent, bitangent, normal); // aka TBN-matrix
	// transform light to TangentSpace:
	vec3 eyeVec = normalize((ModelViewMatrix * vertex).xyz * eyeTangentMatrix);
	lightDir = normalize(sunPosition.xyz * eyeTangentMatrix);
	halfVec = lightDir + eyeVec;

	vec4 position = ModelViewProjectionMatrix * vertex;
	vertexDistance = position.z;
	gl_Position = position;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
