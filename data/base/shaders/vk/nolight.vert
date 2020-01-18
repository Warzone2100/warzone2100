#version 450
//#pragma debug(on)

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

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 vertexTexCoord;

layout(location = 0) out vec2 texCoord;

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoord;

	// Translate every vertex according to the Model, View and Projection matrices
	gl_Position = ModelViewProjectionMatrix * vertex;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
