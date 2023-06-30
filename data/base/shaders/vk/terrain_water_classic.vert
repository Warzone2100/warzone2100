#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUVLightmapMatrix;
	mat4 ShadowMapMVPMatrix;
	mat4 ModelUV1Matrix;
	mat4 ModelUV2Matrix;
	vec4 cameraPos; // in modelSpace
	vec4 sunPos; // in modelSpace, normalized
	vec4 fogColor;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	float timeSec;
};

layout(location = 0) in vec4 vertex; // .w is depth

layout(location = 1) out vec2 uvLightmap;
layout(location = 2) out vec2 uv1;
layout(location = 3) out vec2 uv2;
layout(location = 4) out float vertexDistance;
layout(location = 5) out float depth;

void main()
{
	uvLightmap = (ModelUVLightmapMatrix * vec4(vertex.xyz, 1.f)).xy;
	depth = vertex.w;

	uv1 = vec2(vertex.x/4.f/128.f + timeSec/80.f, -vertex.z/4.f/128.f + timeSec/40.f); // (ModelUV1Matrix * vertex).xy;
	uv2 = vec2(vertex.x/4.f/128.f + timeSec/80.f, -vertex.z/4.f/128.f + timeSec/10.f); // (ModelUV2Matrix * vertex).xy;

	vec4 position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.f);
	vertexDistance = position.z;
	gl_Position = position;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
