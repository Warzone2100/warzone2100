// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 ModelViewMatrix;
uniform mat4 ModelViewProjectionMatrix;
uniform mat4 NormalMatrix;

uniform vec4 sunPosition;

uniform vec4 paramx1;
uniform vec4 paramy1;
uniform vec4 paramx2;
uniform vec4 paramy2;

uniform mat4 textureMatrix1;
uniform mat4 textureMatrix2;

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
out vec2 uv1;
out vec2 uv2;
out float vertexDistance;
// In tangent space
out vec3 lightDir;
out vec3 halfVec;
#else
varying vec4 color;
varying vec2 uv1;
varying vec2 uv2;
varying float vertexDistance;
// In tangent space
varying vec3 lightDir;
varying vec3 halfVec;
#endif

void main()
{
	color = vertexColor;
	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	vec4 uv1_tmp = textureMatrix1 * vec4(dot(paramx1, vertex), dot(paramy1, vertex), 0., 1.);
	uv1 = uv1_tmp.xy / uv1_tmp.w;
	vec4 uv2_tmp = textureMatrix2 * vec4(dot(paramx2, vertex), dot(paramy2, vertex), 0., 1.);
	uv2 = uv2_tmp.xy / uv2_tmp.w;
	vertexDistance = position.z;

	// constructing EyeSpace -> TangentSpace mat3
	mat3 normalMat = mat3(NormalMatrix);
	vec3 normal = normalize(normalMat * vertexNormal);
	vec3 tangent = normalize(cross(normal, normalMat * paramy1.xyz));
	vec3 bitangent = cross(normal, tangent);
	mat3 eyeTangentMatrix = mat3(tangent, bitangent, normal); // aka TBN-matrix
	// transform light to TangentSpace:
	vec3 eyeVec = normalize((ModelViewMatrix * vertex).xyz * eyeTangentMatrix);
	lightDir = normalize(sunPosition.xyz * eyeTangentMatrix);
	halfVec = lightDir + eyeVec;
}
