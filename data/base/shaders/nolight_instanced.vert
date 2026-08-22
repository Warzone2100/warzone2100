// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

#include "tcmask_instanced.glsl"

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#if !defined(NEWGL) && defined(GL_EXT_gpu_shader4)
#extension GL_EXT_gpu_shader4 : enable
#endif


#if defined(NEWGL) || defined(GL_EXT_gpu_shader4)
#define intMod(a, b) a % b
#else
#define intMod(a, b) floor((a - floor((a + 0.5) / b) * b) + 0.5)
#endif

#ifdef NEWGL
#define VERTEX_INPUT in
#define VERTEX_OUTPUT out
#else
#define VERTEX_INPUT attribute
#define VERTEX_OUTPUT varying
#endif

VERTEX_INPUT vec4 vertex;
VERTEX_INPUT vec4 vertexTexCoordAndTexAnim;
VERTEX_INPUT mat4 instanceModelMatrix;
VERTEX_INPUT vec4 instancePackedValues; // shaderStretch_ecmState_alphaTest_animFrameNumber
VERTEX_INPUT vec4 instanceColour;
VERTEX_INPUT vec4 instanceTeamColour; // not needed?

VERTEX_OUTPUT vec4 texCoord_vertexDistance; // vec(2) texCoord, float vertexDistance, (unused float)
VERTEX_OUTPUT vec4 colour;
VERTEX_OUTPUT vec4 packed_ecmState_alphaTest;
VERTEX_OUTPUT vec3 posViewSpace;

void main()
{
	// unpack inputs
	mat4 ModelViewMatrix = ViewMatrix * instanceModelMatrix;
	float alphaTest = instancePackedValues.z;
	float animFrameNumber = instancePackedValues.w;

	// Pass texture coordinates to fragment shader
	vec2 texCoord = vertexTexCoordAndTexAnim.xy;
	int framesPerLine = int(1.f / min(vertexTexCoordAndTexAnim.z, 1.f)); // texAnim.x
	int frame = int(animFrameNumber);
	float uFrame = float(intMod(frame, framesPerLine)) * vertexTexCoordAndTexAnim.z; // texAnim.x
	float vFrame = float(frame / framesPerLine) * vertexTexCoordAndTexAnim.w; // texAnim.y
	texCoord = vec2(texCoord.x + uFrame, texCoord.y + vFrame);

	// Translate every vertex according to the Model View and Projection Matrix
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	vec4 gposition = ModelViewProjectionMatrix * vertex;
	gl_Position = gposition;

	posViewSpace = (ModelViewMatrix * vertex).xyz;

	// pack outputs for fragment shader
	colour = instanceColour;
	texCoord_vertexDistance = vec4(texCoord.x, texCoord.y, 0.f, 0.f);
	packed_ecmState_alphaTest = vec4(0.f, alphaTest, 0.f, 0.f);
}
