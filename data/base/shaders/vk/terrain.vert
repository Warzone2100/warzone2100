#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 textureMatrix1;
	mat4 textureMatrix2;
	mat4 ModelViewMatrix;
	mat4 ModelViewProjectionMatrix;
	vec4 sunPosition;
	vec4 paramx1;
	vec4 paramy1;
	vec4 paramx2;
	vec4 paramy2;
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

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 uv1;
layout(location = 2) out vec2 uv2;
layout(location = 3) out float vertexDistance;

void main()
{
	color = vertexColor;
	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	vec4 uv1_tmp = textureMatrix1 * vec4(dot(paramx1, vertex), dot(paramy1, vertex), 0., 1.);
	uv1 = uv1_tmp.xy / uv1_tmp.w;
	vec4 uv2_tmp = textureMatrix2 * vec4(dot(paramx2, vertex), dot(paramy2, vertex), 0., 1.);
	uv2 = uv2_tmp.xy / uv2_tmp.w;
	vertexDistance = position.z;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
