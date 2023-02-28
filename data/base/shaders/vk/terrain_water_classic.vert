#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUVLightmapMatrix;
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
	uvLightmap = (ModelUVLightmapMatrix * vec4(vertex.xyz, 1)).xy;
	depth = vertex.w;

	uv1 = vec2(vertex.x/4/128 + timeSec/80, -vertex.z/4/128 + timeSec/40); // (ModelUV1Matrix * vertex).xy;
	uv2 = vec2(vertex.x/4/128 + timeSec/80, -vertex.z/4/128 + timeSec/10); // (ModelUV2Matrix * vertex).xy;

	vec4 position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1);
	vertexDistance = position.z;
	gl_Position = position;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
