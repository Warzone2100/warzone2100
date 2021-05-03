// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelUV1Matrix;
uniform mat4 ModelUV2Matrix;

uniform float time; // in seconds

uniform vec4 cameraPos; // in modelSpace
uniform vec4 sunPos; // in modelSpace, normalized

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex; // w is depth
#else
attribute vec4 vertex;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec2 uv1;
out vec2 uv2;
out float depth;
out float vertexDistance;
// light in modelSpace:
out vec3 lightDir;
out vec3 halfVec;
#else
varying vec2 uv1;
varying vec2 uv2;
varying float depth;
varying float vertexDistance;
varying vec3 lightDir;
varying vec3 halfVec;
#endif

void main()
{
	depth = vertex.w;
	vec4 position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1);
	gl_Position = position;
	vertexDistance = position.z;

	uv1 = vec2(vertex.x/4/128 + time/10, -vertex.z/4/128); // (ModelUV1Matrix * vertex).xy;
	uv2 = vec2(vertex.x/5/128, -vertex.z/5/128); // (ModelUV2Matrix * vertex).xy;

	vec3 eyeVec = normalize(cameraPos.xyz - vertex.xyz);
	lightDir = sunPos.xyz;
	halfVec = lightDir + eyeVec;
}
