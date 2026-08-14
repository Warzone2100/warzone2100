// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#if !defined(NEWGL) && defined(GL_EXT_gpu_shader4)
#extension GL_EXT_gpu_shader4 : enable
#endif

layout(std140) uniform globaluniforms {
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ShadowMapMVPMatrix;
	vec4 cameraPos;
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
	float WZ_MIP_LOAD_BIAS;
};

layout(std140) uniform meshuniforms {
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
};

layout(std140) uniform instanceuniforms {
	mat4 ModelMatrix;
	mat4 NormalMatrix;
	vec4 colour;
	vec4 teamcolour;
	float stretch;
	float animFrameNumber;
	int ecmEffect;
	int alphaTest;
};

#if defined(NEWGL) || defined(GL_EXT_gpu_shader4)
#define intMod(a, b) a % b
#else
#define intMod(a, b) floor((a - floor((a + 0.5) / b) * b) + 0.5)
#endif

#ifdef NEWGL
in vec4 vertex;
in vec4 vertexTexCoordAndTexAnim;
#else
attribute vec4 vertex;
attribute vec4 vertexTexCoordAndTexAnim;
#endif

#ifdef NEWGL
out vec2 texCoord;
out float vertexDistance;
#else
varying vec2 texCoord;
varying float vertexDistance;
#endif

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoordAndTexAnim.xy;
	int framesPerLine = int(1.f / min(vertexTexCoordAndTexAnim.z, 1.f)); // texAnim.x
	int frame = int(animFrameNumber);
	float uFrame = float(intMod(frame, framesPerLine)) * vertexTexCoordAndTexAnim.z; // texAnim.x
	float vFrame = float(frame / framesPerLine) * vertexTexCoordAndTexAnim.w; // texAnim.y
	texCoord = vec2(texCoord.x + uFrame, texCoord.y + vFrame);

	// Translate every vertex according to the Model View and Projection Matrix
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix;
	vec4 gposition = ModelViewProjectionMatrix * vertex;
	gl_Position = gposition;

	// Remember vertex distance
	vertexDistance = gposition.z;
}
