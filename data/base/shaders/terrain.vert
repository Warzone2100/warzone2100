// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 ModelViewMatrix;
uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelViewNormalMatrix;

uniform vec4 sunPosition;

uniform mat4 ModelUVMatrix;
uniform mat4 ModelUVLightMatrix;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
in vec4 vertexColor;
in vec3 vertexNormal;
#else
attribute vec4 vertex;
attribute vec4 vertexColor;
attribute vec3 vertexNormal;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 color;
out vec2 uv;
out vec2 uvLight;
out float vertexDistance;
// In tangent space
out vec3 lightDir;
out vec3 halfVec;
#else
varying vec4 color;
varying vec2 uv;
varying vec2 uvLight;
varying float vertexDistance;
// In tangent space
varying vec3 lightDir;
varying vec3 halfVec;
#endif

void main()
{
	color = vertexColor;

	uv = (ModelUVMatrix * vertex).xy;
	uvLight = (ModelUVLightMatrix * vertex).xy;

	// constructing EyeSpace -> TangentSpace mat3
	mat3 normalMat = mat3(ModelViewNormalMatrix);
	vec3 normal = normalize(normalMat * vertexNormal);
	vec3 vaxis = transpose(ModelUVMatrix)[1].xyz;
	vec3 tangent = normalize(cross(normal, normalMat * vaxis));
	vec3 bitangent = cross(normal, tangent);
	mat3 eyeTangentMatrix = mat3(tangent, bitangent, normal); // aka TBN-matrix
	// transform light to TangentSpace:
	vec3 eyeVec = normalize((ModelViewMatrix * vertex).xyz * eyeTangentMatrix);
	lightDir = normalize(sunPosition.xyz * eyeTangentMatrix);
	halfVec = lightDir + eyeVec;

	vec4 position = ModelViewProjectionMatrix * vertex;
	vertexDistance = position.z;
	gl_Position = position;
}
