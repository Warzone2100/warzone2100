// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#if !defined(NEWGL) && defined(GL_EXT_gpu_shader4)
#extension GL_EXT_gpu_shader4 : enable
#endif

layout(std140) uniform globaluniforms {
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
};

#ifdef NEWGL
#define VERTEX_INPUT in
#else
#define VERTEX_INPUT attribute
#endif

VERTEX_INPUT vec4 vertex;
VERTEX_INPUT vec3 vertexNormal;
VERTEX_INPUT mat4 instanceModelMatrix;
VERTEX_INPUT vec4 instancePackedValues; // shaderStretch_ecmState_alphaTest_animFrameNumber
VERTEX_INPUT vec4 instanceColour;
VERTEX_INPUT vec4 instanceTeamColour;

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
}
