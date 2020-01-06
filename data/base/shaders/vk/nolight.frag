#version 450
//#pragma debug(on)

layout(set = 1, binding = 0) uniform sampler2D Texture;
layout(std140, set = 0, binding = 0) uniform cbuffer
{
	vec4 colour;
	vec4 teamcolour; // the team colour of the model
	float stretch;
	int tcmask; // whether a tcmask texture exists for the model
	int fogEnabled; // whether fog is enabled
	int normalmap; // whether a normal map exists for the model
	int specularmap; // whether a specular map exists for the model
	bool ecmEffect; // whether ECM special effect is enabled
	int alphaTest;
	float graphicsCycle; // a periodically cycling value for special effects
	mat4 ModelViewMatrix;
	mat4 ModelViewProjectionMatrix;
	mat4 NormalMatrix;
	vec4 lightPosition;
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float fogEnd;
	float fogStart;
	vec4 fogColor;
	int hasTangents;
};

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 texColour = texture(Texture, texCoord);

	vec4 fragColour = texColour * colour;

	if (alphaTest > 0 && (fragColour.a <= 0.001))
	{
		discard;
	}

	FragColor = fragColour;
}
