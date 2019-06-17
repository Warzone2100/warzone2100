#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	vec4 paramxlight;
	vec4 paramylight;
	mat4 lightTextureMatrix;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	vec4 fogColor;
};

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 vertexTexCoord;

layout(location = 0) out vec2 uv_tex;
layout(location = 1) out vec2 uv_lightmap;
layout(location = 2) out float vertexDistance;

void main()
{
	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	uv_tex = vertexTexCoord;
	vec4 uv_lightmap_tmp = lightTextureMatrix * vec4(dot(paramxlight, vertex), dot(paramylight, vertex), 0.0, 1.0);
	uv_lightmap = uv_lightmap_tmp.xy / uv_lightmap_tmp.w;
	vertexDistance = position.z;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
