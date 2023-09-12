// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#if !defined(NEWGL) && defined(GL_EXT_gpu_shader4)
#extension GL_EXT_gpu_shader4 : enable
#endif

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;

#if defined(NEWGL) || defined(GL_EXT_gpu_shader4)
#define intMod(a, b) a % b
#else
#define intMod(a, b) floor((a - floor((a + 0.5) / b) * b) + 0.5)
#endif

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

float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
	// unpack inputs
	mat4 ModelViewMatrix = ViewMatrix * instanceModelMatrix;
	float stretch = instancePackedValues.x;

	// Implement building stretching to accommodate terrain
	vec4 position = vertex;
	if (vertex.y <= 0.0) // use vertex here directly to help shader compiler optimization
	{
		// NOTE: 'stretch' may be:
		//	- if positive: building stretching
		//	- if negative: the height above the terrain of the model intance overall
		position.y -= (stretch * when_gt(stretch, 0.f));
	}

	// Translate every vertex according to the Model View and Projection Matrix
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	vec4 gposition = ModelViewProjectionMatrix * position;
	gl_Position = gposition;
}
