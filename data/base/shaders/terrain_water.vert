// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelUVLightmapMatrix;
uniform mat4 ModelUV1Matrix;
uniform mat4 ModelUV2Matrix;

uniform float timeSec;

uniform vec4 cameraPos; // in modelSpace
uniform vec4 sunPos; // in modelSpace, normalized

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex; // w is depth
#else
attribute vec4 vertex;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 uv1_uv2;
out vec2 uvLightmap;
out float depth;
out float depth2;
out float vertexDistance;
// light in modelSpace:
out vec3 lightDir;
out vec3 halfVec;
#else
varying vec4 uv1_uv2;
varying vec2 uvLightmap;
varying float depth;
varying float depth2;
varying float vertexDistance;
varying vec3 lightDir;
varying vec3 halfVec;
#endif

void main()
{
	uvLightmap = (ModelUVLightmapMatrix * vec4(vertex.xyz, 1)).xy;
	depth = vertex.w;
	depth2 = length(vertex.y - vertex.w);

	vec2 uv1 = vec2(vertex.x/4/128 + timeSec/80, -vertex.z/4/128 + timeSec/40); // (ModelUV1Matrix * vertex).xy;
	vec2 uv2 = vec2(vertex.x/4/128, -vertex.z/4/128 - timeSec/40); // (ModelUV2Matrix * vertex).xy;
	uv1_uv2 = vec4(uv1.x, uv1.y, uv2.x, uv2.y);

	vec3 eyeVec = normalize(cameraPos.xyz - vertex.xyz);
	lightDir = sunPos.xyz;
	halfVec = lightDir + eyeVec;

	vec4 position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1);
	vertexDistance = position.z;
	gl_Position = position;
}
