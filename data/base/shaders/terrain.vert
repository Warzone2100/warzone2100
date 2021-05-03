// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)
uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelUVMatrix;
uniform mat4 ModelUVLightMatrix;

uniform vec4 cameraPos; // in modelSpace
uniform vec4 sunPos; // in modelSpace, normalized

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

	// constructing ModelSpace -> TangentSpace mat3
	vec3 vaxis = transpose(ModelUVMatrix)[1].xyz;
	vec3 tangent = normalize(cross(vertexNormal, vaxis));
	vec3 bitangent = cross(vertexNormal, tangent);
	mat3 ModelTangentMatrix = mat3(tangent, bitangent, vertexNormal); // aka TBN-matrix
	// transform light to TangentSpace:
	vec3 eyeVec = normalize((cameraPos.xyz - vertex.xyz) * ModelTangentMatrix);
	lightDir = sunPos.xyz * ModelTangentMatrix; // already normalized
	halfVec = lightDir + eyeVec;

	vec4 position = ModelViewProjectionMatrix * vertex;
	vertexDistance = position.z;
	gl_Position = position;
}
