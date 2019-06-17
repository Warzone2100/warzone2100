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
};

layout(location = 0) in vec4 vertex;
layout(location = 3) in vec3 vertexNormal;
layout(location = 1) in vec2 vertexTexCoord;

layout(location = 0) out float vertexDistance;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 lightDir;
layout(location = 3) out vec3 eyeVec;
layout(location = 4) out vec2 texCoord;

void main()
{
	vec3 vVertex = (ModelViewMatrix * vertex).xyz;
	vec4 position = vertex;

	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoord;

	// Lighting -- we pass these to the fragment shader
	normal = (NormalMatrix * vec4(vertexNormal, 0.0)).xyz;
	lightDir = lightPosition.xyz - vVertex;
	eyeVec = -vVertex;

	// Implement building stretching to accomodate terrain
	if (vertex.y <= 0.0) // use vertex here directly to help shader compiler optimization
	{
		position.y -= stretch;
	}

	// Translate every vertex according to the Model View and Projection Matrix
	vec4 gposition = ModelViewProjectionMatrix * position;
	gl_Position = gposition;

	// Remember vertex distance
	vertexDistance = gposition.z;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
