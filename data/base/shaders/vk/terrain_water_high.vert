#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUVLightmapMatrix;
	mat4 ModelUV1Matrix;
	mat4 ModelUV2Matrix;
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
	float timeSec;
};

layout(location = 0) in vec4 vertex; // .w is depth

layout(location = 1) out vec4 uv1_uv2;
layout(location = 2) out vec2 uvLightmap;
layout(location = 3) out float vertexDistance;
layout(location = 4) out vec3 lightDir;
layout(location = 5) out vec3 halfVec;
layout(location = 6) out float depth;
layout(location = 7) out float depth2;

void main()
{
	uvLightmap = (ModelUVLightmapMatrix * vec4(vertex.xyz, 1.f)).xy;
	depth = vertex.w;
	depth2 = length(vertex.y - vertex.w);

	vec2 uv1 = vec2(vertex.x/4.f/128.f + timeSec/80.f, -vertex.z/4.f/128.f + timeSec/40.f); // (ModelUV1Matrix * vertex).xy;
	vec2 uv2 = vec2(vertex.x/4.f/128.f, -vertex.z/4.f/128.f - timeSec/40.f); // (ModelUV2Matrix * vertex).xy;
	uv1_uv2 = vec4(uv1.x, uv1.y, uv2.x, uv2.y);

	vec3 eyeVec = normalize(cameraPos.xyz - vertex.xyz);
	lightDir = sunPos.xyz;
	halfVec = lightDir + eyeVec;

	vec4 position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.f);
	vertexDistance = position.z;
	gl_Position = position;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
