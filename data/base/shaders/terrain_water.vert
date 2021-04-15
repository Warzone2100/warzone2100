// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 ModelViewProjectionMatrix;
uniform mat3 ModelTangentMatrix;
uniform mat4 ModelUV1;
uniform mat4 ModelUV2;

uniform vec3 cameraPos;
uniform vec3 lightDirInTangent;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
#else
attribute vec4 vertex;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec2 uv1;
out vec2 uv2;
out float vertexDistance;
// light in tangent space:
out vec3 lightDir;
out vec3 halfVec;
#else
varying vec2 uv1;
varying vec2 uv2;
varying float vertexDistance;
varying vec3 lightDir;
varying vec3 halfVec;
#endif

void main()
{
	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	uv1 = (ModelUV1 * vertex).xy;
	uv2 = (ModelUV2 * vertex).xy;
	vertexDistance = position.z;

	// transform light to TangentSpace:
	vec3 eyeVec = normalize(ModelTangentMatrix * (cameraPos - vertex.xyz));
	lightDir = lightDirInTangent;
	halfVec = lightDir + eyeVec;
}
