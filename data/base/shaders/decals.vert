// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelUVLightmapMatrix;

uniform vec4 cameraPos; // in modelSpace
uniform vec4 sunPos; // in modelSpace, normalized

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#ifdef NEWGL
in vec4 vertex;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexTangent;
#else
attribute vec4 vertex;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexTangent;
#endif

#ifdef NEWGL
out vec2 uv_tex;
out vec2 uv_lightmap;
out float vertexDistance;
// In tangent space
out vec3 lightDir;
out vec3 halfVec;
#else
varying vec2 uv_tex;
varying vec2 uv_lightmap;
varying float vertexDistance;
// In tangent space
varying vec3 lightDir;
varying vec3 halfVec;
#endif

void main()
{
	uv_tex = vertexTexCoord;
	uv_lightmap = (ModelUVLightmapMatrix * vertex).xy;

	// constructing ModelSpace -> TangentSpace mat3
	vec3 bitangent = cross(vertexNormal, vertexTangent.xyz) * vertexTangent.w;
	mat3 ModelTangentMatrix = mat3(vertexTangent.xyz, bitangent, vertexNormal);
	// transform light from ModelSpace to TangentSpace:
	vec3 eyeVec = normalize((cameraPos.xyz - vertex.xyz) * ModelTangentMatrix);
	lightDir = sunPos.xyz * ModelTangentMatrix;
	halfVec = lightDir + eyeVec;

	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	vertexDistance = position.z;
}
