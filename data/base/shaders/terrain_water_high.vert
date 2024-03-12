// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelUVLightmapMatrix;
uniform mat4 ModelUV1Matrix;
uniform mat4 ModelUV2Matrix;

uniform float timeSec;

uniform vec4 cameraPos; // in modelSpace
uniform vec4 sunPos; // in modelSpace, normalized

#ifdef NEWGL
#define VERTEX_INPUT in
#define VERTEX_OUTPUT out
#else
#define VERTEX_INPUT attribute
#define VERTEX_OUTPUT varying
#endif

VERTEX_INPUT vec4 vertex; // w is depth

VERTEX_OUTPUT vec4 uv1_uv2;
VERTEX_OUTPUT vec2 uvLightmap;
VERTEX_OUTPUT float depth;
VERTEX_OUTPUT float depth2;
VERTEX_OUTPUT float vertexDistance;
// light in modelSpace:
VERTEX_OUTPUT vec3 lightDir;
VERTEX_OUTPUT vec3 halfVec;

void main()
{
	uvLightmap = (ModelUVLightmapMatrix * vec4(vertex.xyz, 1.f)).xy;
	depth = vertex.w;
	depth2 = length(vertex.y - vertex.w);

	vec2 uv1 = vec2(vertex.x/4.f/128.f + timeSec/80.f, -vertex.z/4.f/128.f + timeSec/40.f); // (ModelUV1Matrix * vertex).xy;
	vec2 uv2 = vec2(vertex.x/4.f/128.f, -vertex.z/4.f/128.f - timeSec/40.f); // (ModelUV2Matrix * vertex).xy;
	uv1_uv2 = vec4(uv1.x, uv1.y, uv2.x, uv2.y);

	vec3 eyeVec = normalize(cameraPos.xyz - vertex.xyz);
	lightDir = sunPos.xyz;
	halfVec = lightDir + eyeVec;

	vec4 position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.f);
	vertexDistance = position.z;
	gl_Position = position;
}
