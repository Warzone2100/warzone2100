#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 invProjectionMatrix;
	mat4 projectionMatrix;
	vec4 params;
	vec2 noiseScale;
	vec2 padding;
	vec4 kernel[16];
	vec4 uvScaleClamp;
};

layout(set = 1, binding = 0) uniform sampler2D depthTexture;
layout(set = 1, binding = 1) uniform sampler2D normalsTexture;
layout(set = 1, binding = 2) uniform sampler2D noiseTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

#include "ssao_view_common.glsl"

const int KERNEL_SIZE = 16;
const float SKY_DEPTH_THRESHOLD = 0.9999;

vec3 getViewNormal(vec2 uv)
{
	vec3 n = texture(normalsTexture, uv).xyz * 2.0 - 1.0;
	float len = length(n);
	if (len < 1e-5)
	{
		return vec3(0.0, 0.0, 1.0);
	}
	return n / len;
}

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	float depth = texture(depthTexture, uv).r;
	if (depth >= SKY_DEPTH_THRESHOLD)
	{
		FragColor = vec4(1.0);
		return;
	}

	vec3 origin = wzGetViewPosition(uv, depth, invProjectionMatrix);
	vec3 normal = getViewNormal(uv);

	vec2 noiseUV = uv * noiseScale;
	vec3 randomVec = normalize(texture(noiseTexture, noiseUV).xyz * 2.0 - 1.0);

	vec3 tangent = normalize(randomVec - normal * dot(normal, randomVec));
	vec3 bitangent = cross(normal, tangent);
	mat3 tbn = mat3(tangent, bitangent, normal);

	float radius = params.x * abs(origin.z);
	float bias = max(params.y * abs(origin.z), params.z);
	float maxDepthDiff = radius * params.w;
	float occlusion = 0.0;
	for (int i = 0; i < KERNEL_SIZE; ++i)
	{
		vec3 samplePos = origin + (tbn * kernel[i].xyz) * radius;

		vec4 offset = projectionMatrix * vec4(samplePos, 1.0);
		offset.xyz /= offset.w;
		vec2 sampleUV = vec2(offset.x, -offset.y) * 0.5 + 0.5;

		if (sampleUV.x < 0.0 || sampleUV.x > uvScaleClamp.z || sampleUV.y < 0.0 || sampleUV.y > uvScaleClamp.w)
		{
			continue;
		}

		sampleUV = clamp(sampleUV, vec2(0.0), uvScaleClamp.zw);
		float sampleDepth = texture(depthTexture, sampleUV).r;
		if (sampleDepth >= SKY_DEPTH_THRESHOLD)
		{
			continue;
		}

		vec3 sampleViewPos = wzGetViewPosition(sampleUV, sampleDepth, invProjectionMatrix);
		float depthDiff = abs(origin.z - sampleViewPos.z);
		float rangeCheck = 1.0 - smoothstep(0.0, maxDepthDiff, depthDiff);

		// Attenuate each hit by N*occDir (actual occluder direction, not kernel offset).
		// Occluders below the tangent plane contribute less, reducing false self-occlusion
		// on flat mesh hulls. Still normalize by KERNEL_SIZE (LearnOpenGL-style) so sparse
		// hits cannot drive occlusion to zero and blow up contact shadows after blur.
		vec3 occDelta = sampleViewPos - origin;
		float occDistSq = dot(occDelta, occDelta);
		float angleWeight = (occDistSq >= 1e-8)
			? max(dot(normal, occDelta * inversesqrt(occDistSq)), 0.0)
			: 0.0;

		// pie_PerspectiveGet uses +Z into the scene; LearnOpenGL's >= test assumes -Z forward.
		float hit = (sampleViewPos.z <= samplePos.z - bias) ? 1.0 : 0.0;
		occlusion += hit * rangeCheck * angleWeight;
	}

	occlusion = 1.0 - (occlusion / float(KERNEL_SIZE));
	FragColor = vec4(vec3(occlusion), 1.0);
}
