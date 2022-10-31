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

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 vertexTexCoordAndTexAnim;
layout(location = 5) in mat4 instanceModelMatrix;
layout(location = 9) in vec4 instancePackedValues; // shaderStretch_ecmState_alphaTest_animFrameNumber
layout(location = 10) in vec4 instanceColour;
layout(location = 11) in vec4 instanceTeamColour;

layout(location = 0) out vec4 texCoord_vertexDistance; // vec(2) texCoord, float vertexDistance, (unused float)
layout(location = 1) out vec4 colour;
layout(location = 2) out vec4 packed_ecmState_alphaTest;

void main()
{
	// unpack inputs
	#define ModelViewMatrix instanceModelMatrix
	float alphaTest = instancePackedValues.z;
	float animFrameNumber = instancePackedValues.w;

	// Pass texture coordinates to fragment shader
	vec2 texCoord = vertexTexCoordAndTexAnim.xy;
	int framesPerLine = int(1.f / min(vertexTexCoordAndTexAnim.z, 1.f)); // texAnim.x
	int frame = int(animFrameNumber);
	float uFrame = float(frame % framesPerLine) * vertexTexCoordAndTexAnim.z; // texAnim.x
	float vFrame = float(frame / framesPerLine) * vertexTexCoordAndTexAnim.w; // texAnim.y
	texCoord = vec2(texCoord.x + uFrame, texCoord.y + vFrame);

	// Translate every vertex according to the Model, View and Projection matrices
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	vec4 gposition = ModelViewProjectionMatrix * vertex;
	gl_Position = gposition;

	// Remember vertex distance
	float vertexDistance = gposition.z;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;

	// pack outputs for fragment shader
	colour = instanceColour;
	texCoord_vertexDistance = vec4(texCoord.x, texCoord.y, vertexDistance, 0.f);
	packed_ecmState_alphaTest = vec4(0.f, alphaTest, 0.f, 0.f);
}
