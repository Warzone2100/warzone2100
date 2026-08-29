#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	vec4 uvScaleClamp;
};

layout(set = 1, binding = 0) uniform sampler2D sceneTexture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	vec4 scene = texture(sceneTexture, uv);

	// Pass-through until the scene format is floating point
	vec3 result = scene.rgb;

	FragColor = vec4(result, scene.a);
}
