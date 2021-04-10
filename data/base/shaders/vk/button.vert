#version 450
//#pragma debug(on)

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

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 vertexTexCoord;

layout(location = 0) out vec2 texCoord;

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoord;

	// Translate every vertex according to the Model, View and Projection matrices
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	gl_Position = ModelViewProjectionMatrix * vertex;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
