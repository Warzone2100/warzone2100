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
	int hasTangents;
};

layout(location = 0) in vec4 vertex;
layout(location = 3) in vec3 vertexNormal;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 4) in vec4 vertexTangent;

layout(location = 0) out float vertexDistance;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 lightDir;
layout(location = 3) out vec3 halfVec;
layout(location = 4) out vec2 texCoord;

void main()
{
	vec3 vVertex = normalize((ModelViewMatrix * vertex).xyz);
	vec4 position = vertex;

	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoord;

	// Lighting -- we pass these to the fragment shader
	vec3 n = normalize((NormalMatrix * vec4(vertexNormal, 0.0)).xyz);
	vec3 eyeVec = -vVertex;
	lightDir = normalize(lightPosition.xyz - vVertex);

	if (hasTangents != 0)
	{
		// Building the matrix Eye Space -> Tangent Space with handness
		vec3 t = normalize((NormalMatrix * vertexTangent).xyz);
		vec3 b = cross (n, t) * vertexTangent.w;
		mat3 TangentSpaceMatrix = mat3(t, n, b);

		// Transform calculated normals for vanilla models by tangent basis
		n = n * TangentSpaceMatrix;

		// Transform light and eye direction vectors by tangent basis
		lightDir *= TangentSpaceMatrix;
		eyeVec *= TangentSpaceMatrix;
	}

	normal = n;
	halfVec = normalize(lightDir - eyeVec);

	// Implement building stretching to accommodate terrain
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
