#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	float ssaoIntensity;
	vec4 uvScaleClamp;
};

layout(set = 1, binding = 0) uniform sampler2D sceneTexture;
layout(set = 1, binding = 1) uniform sampler2D ssaoTexture;
layout(set = 1, binding = 2) uniform sampler2D prepassNormals;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	vec3 scene = texture(sceneTexture, uv).rgb;
	float ao = texture(ssaoTexture, uv).r;
	float weight = texture(prepassNormals, uv).a;
	vec3 litAo = scene * mix(1.0, ao, ssaoIntensity * weight);
	FragColor = vec4(litAo, 1.0);
}
