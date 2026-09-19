#version 450
//#pragma debug(on)

layout(set = 3, binding = 0) uniform sampler2D Texture;

layout(std140, set = 0, binding = 0) uniform globaluniforms
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ShadowMapMVPMatrix;
	vec4 cameraPos;
	vec4 lightPosition; // in world space
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 fogColor;
	vec4 fogRange;
	float graphicsCycle;
	float WZ_MIP_LOAD_BIAS;
	float pad0;
	float pad1;
};

layout(std140, set = 1, binding = 0) uniform meshuniforms
{
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
	int fogOutput;
};

layout(std140, set = 2, binding = 0) uniform instanceuniforms
{
	mat4 ModelMatrix;
	mat4 NormalMatrix;
	vec4 colour;
	vec4 teamcolour;
	float stretch;
	float animFrameNumber;
	int ecmEffect;
	int alphaTest;
};

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 posViewSpace;

layout(location = 0) out vec4 FragColor;

#include "distance_fog.glsl"

void main()
{
	vec4 texColour = texture(Texture, texCoord, WZ_MIP_LOAD_BIAS);

	vec4 fragColour = texColour * colour;
	if (fogRange.z > 0.5 && fogOutput != WZ_FOG_OUTPUT_DISABLED)
	{
		float fogAmount = wzDistanceFogAmount(length(posViewSpace), fogRange.x, fogRange.y);
		fragColour.rgb = wzApplyForwardFog(fragColour.rgb, fragColour.a, fogAmount, fogColor.rgb, fogOutput);
	}

	if (alphaTest > 0 && (fragColour.a <= 0.001))
	{
		discard;
	}

	FragColor = fragColour;
}
