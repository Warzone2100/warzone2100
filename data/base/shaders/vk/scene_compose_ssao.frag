#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	float ssaoIntensity;
	float padding0;
	float padding1;
	float padding2;
	vec4 sceneUvScaleClamp;
	vec4 aoUvScaleClamp;
};

layout(set = 1, binding = 0) uniform sampler2D sceneTexture;
layout(set = 1, binding = 1) uniform sampler2D ssaoTexture;
layout(set = 1, binding = 2) uniform sampler2D prepassNormals;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec2 sceneUv = clamp(texCoords * sceneUvScaleClamp.xy, vec2(0.0), sceneUvScaleClamp.zw);
	vec2 aoUv = clamp(texCoords * aoUvScaleClamp.xy, vec2(0.0), aoUvScaleClamp.zw);
	vec3 scene = texture(sceneTexture, sceneUv).rgb;
	float ao = texture(ssaoTexture, aoUv).r;
	float weight = texture(prepassNormals, sceneUv).a;
	vec3 litAo = scene * mix(1.0, ao, ssaoIntensity * weight);
	FragColor = vec4(litAo, 1.0);
}
