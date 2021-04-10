#version 450
//#pragma debug(on)

layout(set = 3, binding = 0) uniform sampler2D Texture;
layout(set = 3, binding = 1) uniform sampler2D TextureTcmask;
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
	int ecmEffect;
	int alphaTest;
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
