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
	vec4 fogRange;
	float graphicsCycle;
	float WZ_MIP_LOAD_BIAS;
	float pad0;
	float pad1;
};

layout(std140) uniform meshuniforms {
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
	int fogOutput;
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
in vec3 vertexNormal;
in vec4 vertexTexCoordAndTexAnim;
in vec4 vertexTangent;
#else
attribute vec4 vertex;
attribute vec3 vertexNormal;
attribute vec4 vertexTexCoordAndTexAnim;
attribute vec4 vertexTangent;
#endif

#ifdef NEWGL
out vec3 posViewSpace;
out vec3 normal, lightDir, halfVec;
out vec2 texCoord;
out mat3 TangentSpaceMatrix;
#else
varying vec3 posViewSpace;
varying vec3 normal, lightDir, halfVec;
varying vec2 texCoord;
varying mat3 TangentSpaceMatrix;
#endif

#include "mesh_shading_normal.glsl"

float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoordAndTexAnim.xy;
	int framesPerLine = int(1.f / min(vertexTexCoordAndTexAnim.z, 1.f)); // texAnim.x
	int frame = int(animFrameNumber);
	float uFrame = float(intMod(frame, framesPerLine)) * vertexTexCoordAndTexAnim.z; // texAnim.x
	float vFrame = float(frame / framesPerLine) * vertexTexCoordAndTexAnim.w; // texAnim.y
	texCoord = vec2(texCoord.x + uFrame, texCoord.y + vFrame);

	// Lighting, all in WORLD space
	vec3 posWorld = (ModelMatrix * vertex).xyz;
	vec3 cameraVec = normalize(cameraPos.xyz - posWorld);

	normal = wzWorldShadingNormal(mat3(NormalMatrix), vertexNormal, hasTangents);
	lightDir = -normalize(lightPosition.xyz);

	if (hasTangents != 0)
	{
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

	posViewSpace = (ViewMatrix * ModelMatrix * position).xyz;
}
