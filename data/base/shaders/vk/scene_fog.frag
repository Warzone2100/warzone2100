#version 450

#include "ssao_view_common.glsl"

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 fogColor;
	mat4 invProjectionMatrix;
	vec4 uvScaleClamp;
	float fogBegin;
	float fogEnd;
};

layout(set = 1, binding = 0) uniform sampler2D sceneTexture;
layout(set = 1, binding = 1) uniform sampler2D prepassDepth;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

const float SKY_DEPTH_THRESHOLD = 0.9999;

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	vec3 lit = texture(sceneTexture, uv).rgb;
	vec3 result = lit;
	float depth = texture(prepassDepth, uv).r;
	if (depth < SKY_DEPTH_THRESHOLD)
	{
		float fogRange = fogEnd - fogBegin;
		if (abs(fogRange) > 1e-3)
		{
			float viewDist = length(wzGetViewPosition(uv, depth, invProjectionMatrix));
			float f = clamp((viewDist - fogBegin) / fogRange, 0.0, 1.0);
			result = mix(lit, fogColor.rgb, f);
		}
	}
	// Sky / no prepass geometry: skybox already applied its own fog in ScenePass.

	FragColor = vec4(result, 1.0);
}
