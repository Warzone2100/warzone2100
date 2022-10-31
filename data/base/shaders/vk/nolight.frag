#version 450
//#pragma debug(on)

layout (constant_id = 0) const float WZ_MIP_LOAD_BIAS = 0.f;

layout(set = 3, binding = 0) uniform sampler2D Texture;
layout(std140, set = 0, binding = 0) uniform globaluniforms
{
	mat4 ProjectionMatrix;
	vec4 lightPosition;
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 fogColor;
	float fogEnd;
	float fogStart;
	float graphicsCycle;
	int fogEnabled;
};

layout(std140, set = 1, binding = 0) uniform meshuniforms
{
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
};

layout(std140, set = 2, binding = 0) uniform instanceuniforms
{
	mat4 ModelViewMatrix;
	mat4 NormalMatrix;
	vec4 colour;
	vec4 teamcolour;
	float stretch;
	float animFrameNumber;
	int ecmEffect;
	int alphaTest;
};

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 texColour = texture(Texture, texCoord, WZ_MIP_LOAD_BIAS);

	vec4 fragColour = texColour * colour;

	if (alphaTest > 0 && (fragColour.a <= 0.001))
	{
		discard;
	}

	FragColor = fragColour;
}
