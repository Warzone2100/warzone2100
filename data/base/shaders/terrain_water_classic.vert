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

VERTEX_OUTPUT vec2 uvLightmap;
VERTEX_OUTPUT vec2 uv1;
VERTEX_OUTPUT vec2 uv2;
VERTEX_OUTPUT float depth;
VERTEX_OUTPUT float vertexDistance;

void main()
{
	uvLightmap = (ModelUVLightmapMatrix * vec4(vertex.xyz, 1.f)).xy;

	depth = vertex.w;
	vec4 position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.f);
	gl_Position = position;
	vertexDistance = position.z;

	uv1 = vec2(vertex.x/4.f/128.f + timeSec/80.f, -vertex.z/2.f/128.f + timeSec/40.f); // (ModelUV1Matrix * vertex).xy;
	uv2 = vec2(vertex.x/4.f/128.f + timeSec/80.f, -vertex.z/4.f/128.f + timeSec/10.f); // (ModelUV2Matrix * vertex).xy;

}
