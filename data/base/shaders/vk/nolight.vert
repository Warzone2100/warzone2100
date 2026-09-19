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

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 vertexTexCoordAndTexAnim;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec3 posViewSpace;

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoordAndTexAnim.xy;
	int framesPerLine = int(1.f / min(vertexTexCoordAndTexAnim.z, 1.f)); // texAnim.x
	int frame = int(animFrameNumber);
	float uFrame = float(frame % framesPerLine) * vertexTexCoordAndTexAnim.z; // texAnim.x
	float vFrame = float(frame / framesPerLine) * vertexTexCoordAndTexAnim.w; // texAnim.y
	texCoord = vec2(texCoord.x + uFrame, texCoord.y + vFrame);

	// Translate every vertex according to the Model, View and Projection matrices
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix;
	gl_Position = ModelViewProjectionMatrix * vertex;
	posViewSpace = (ViewMatrix * ModelMatrix * vertex).xyz;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
