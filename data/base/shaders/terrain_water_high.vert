// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelUVLightmapMatrix;

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
VERTEX_OUTPUT float vertexDistance;
// light in modelSpace:
VERTEX_OUTPUT vec3 lightDir;
VERTEX_OUTPUT vec3 eyeVec;
VERTEX_OUTPUT vec3 halfVec;
VERTEX_OUTPUT float fresnel;
VERTEX_OUTPUT float fresnel_alpha;

// for Shadows
VERTEX_OUTPUT vec3 fragPos;
//VERTEX_OUTPUT vec3 fragNormal;

void main()
{
	depth = (vertex.w)/96.0;
	depth = 1.0-(clamp(depth, -1.0, 1.0)*0.5+0.5);

	uvLightmap = (ModelUVLightmapMatrix * vec4(vertex.xyz, 1.f)).xy;

	vec2 uv1 = vec2(vertex.x/3.f/128.f, -vertex.z/3.f/128.f + timeSec/45.f); // (ModelUV1Matrix * vertex).xy;
	vec2 uv2 = vec2(vertex.x/4.f/128.f, vertex.z/4.f/128.f + timeSec/60.f); // (ModelUV2Matrix * vertex).xy;
	uv1_uv2 = vec4(uv1.x, uv1.y, uv2.x, uv2.y);

	eyeVec = normalize(cameraPos.xyz - vertex.xyz);
	lightDir = sunPos.xyz;
	halfVec = normalize(lightDir + eyeVec);

	fresnel = dot(eyeVec, halfVec);
	fresnel = pow(1.0 - fresnel, 10.0)*1000.0;
	fresnel_alpha = dot(eyeVec, vec3(0.0, 1.0, 0.0));
	fresnel_alpha = 1.0-fresnel_alpha;
	fresnel_alpha = pow(fresnel_alpha,0.8);
	fresnel_alpha = clamp(fresnel_alpha,0.15, 0.5);

	fragPos = vertex.xyz;
//	fragNormal = vertexNormal;

	vec4 position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.f);
	vertexDistance = position.z;
	gl_Position = position;
}
