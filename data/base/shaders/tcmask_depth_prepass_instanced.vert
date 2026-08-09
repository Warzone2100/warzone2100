// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#if !defined(NEWGL) && defined(GL_EXT_gpu_shader4)
#extension GL_EXT_gpu_shader4 : enable
#endif

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;

#ifdef NEWGL
#define VERTEX_INPUT in
#define VERTEX_OUTPUT out
#else
#define VERTEX_INPUT attribute
#define VERTEX_OUTPUT varying
#endif

VERTEX_INPUT vec4 vertex;
VERTEX_INPUT vec3 vertexNormal;
VERTEX_INPUT mat4 instanceModelMatrix;
VERTEX_INPUT vec4 instancePackedValues; // shaderStretch_ecmState_alphaTest_animFrameNumber
VERTEX_INPUT vec4 instanceColour;
VERTEX_INPUT vec4 instanceTeamColour;

VERTEX_OUTPUT vec3 viewNormal;

float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
	mat4 ModelViewMatrix = ViewMatrix * instanceModelMatrix;
	float stretch = instancePackedValues.x;

	vec4 position = vertex;
	if (vertex.y <= 0.0)
	{
		position.y -= (stretch * when_gt(stretch, 0.f));
	}

	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	gl_Position = ModelViewProjectionMatrix * position;

	// View-space normal from model (no non-uniform scale correction; matches existing mesh path).
	viewNormal = mat3(ModelViewMatrix) * vertexNormal;
}
