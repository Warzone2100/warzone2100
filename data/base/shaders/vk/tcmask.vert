#version 450
//#pragma debug(on)

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

layout(location = 0) in vec4 vertex;
layout(location = 3) in vec3 vertexNormal;
layout(location = 1) in vec4 vertexTexCoordAndTexAnim;
layout(location = 4) in vec4 vertexTangent;

layout(location = 0) out float vertexDistance;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 lightDir;
layout(location = 3) out vec3 halfVec;
layout(location = 4) out vec2 texCoord;
layout(location = 5) out mat3 TangentSpaceMatrix; // occupies locations 5, 6, 7

float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoordAndTexAnim.xy;
	int framesPerLine = int(1.f / min(vertexTexCoordAndTexAnim.z, 1.f)); // texAnim.x
	int frame = int(animFrameNumber);
	float uFrame = float(frame % framesPerLine) * vertexTexCoordAndTexAnim.z; // texAnim.x
	float vFrame = float(frame / framesPerLine) * vertexTexCoordAndTexAnim.w; // texAnim.y
	texCoord = vec2(texCoord.x + uFrame, texCoord.y + vFrame);

	// Lighting, all in WORLD space
	vec3 posWorld = (ModelMatrix * vertex).xyz;
	vec3 cameraVec = normalize(cameraPos.xyz - posWorld);

	// Classic models are lit with the normal negated, which cancels against the
	// negated light below.
	normal = -normalize((NormalMatrix * vec4(vertexNormal, 0.0)).xyz);
	lightDir = -normalize(lightPosition.xyz);

	if (hasTangents != 0)
	{
		// ...but NOT negated when it is the shading normal of a tangent basis.
		normal = normalize((NormalMatrix * vec4(vertexNormal, 0.0)).xyz);

		// Building the World Space -> Tangent Space matrix with handness w to
		// support uv mirroring. The fragment shader applies it.
		vec3 t = normalize((NormalMatrix * vertexTangent).xyz);
		vec3 b = cross (normal, t) * vertexTangent.w;
		TangentSpaceMatrix = mat3(t, b, normal); // conventional (T, B, N)
	}

	halfVec = lightDir + cameraVec;

	// Implement building stretching to accommodate terrain
	vec4 position = vertex;
	if (vertex.y <= 0.0) // use vertex here directly to help shader compiler optimization
	{
		// NOTE: 'stretch' may be:
		//	- if positive: building stretching
		//	- if negative: the height above the terrain of the model instance overall
		position.y -= (stretch * when_gt(stretch, 0.f));
	}

	// Translate every vertex according to the Model View and Projection Matrix
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix;
	vec4 gposition = ModelViewProjectionMatrix * position;
	gl_Position = gposition;

	// Remember vertex distance
	vertexDistance = gposition.z;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
