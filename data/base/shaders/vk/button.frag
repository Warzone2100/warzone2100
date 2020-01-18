#version 450
//#pragma debug(on)

layout(set = 1, binding = 0) uniform sampler2D Texture;
layout(set = 1, binding = 1) uniform sampler2D TextureTcmask;
layout(std140, set = 0, binding = 0) uniform cbuffer
{
	vec4 colour;
	vec4 teamcolour; // the team colour of the model
	float stretch;
	int tcmask; // whether a tcmask texture exists for the model
	int fogEnabled; // whether fog is enabled
	int normalmap; // whether a normal map exists for the model
	int specularmap; // whether a specular map exists for the model
	int ecmEffect; // whether ECM special effect is enabled
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
	vec4 fogColor;
	float fogEnd;
	float fogStart;
	int hasTangents;
};

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 FragColor;

void main()
{
	// Get color from texture unit 0
	vec4 texColour = texture(Texture, texCoord);

	vec4 fragColour;
	if (tcmask == 1)
	{
		// Get tcmask information from texture unit 1
		vec4 mask = texture(TextureTcmask, texCoord);

		// Apply colour using grain merge with tcmask
		fragColour = (texColour + (teamcolour - 0.5) * mask.a) * colour;
	}
	else
	{
		fragColour = texColour * colour;
	}

	if (alphaTest > 0 && fragColour.a <= 0.001)
	{
		discard;
	}

	FragColor = fragColour;
}
