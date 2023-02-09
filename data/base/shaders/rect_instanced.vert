// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// gl_VertexID seems to not be supported on 120, despite documentation to the contrary.

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

uniform mat4 ProjectionMatrix;

#ifdef NEWGL
#define VERTEX_INPUT in
#define VERTEX_OUTPUT out
#else
#define VERTEX_INPUT attribute
#define VERTEX_OUTPUT varying
#endif

VERTEX_INPUT vec4 vertex;
VERTEX_INPUT mat4 instanceModelMatrix;
VERTEX_INPUT vec4 instancePackedValues; // vec2 (offset), vec2 (scale)
VERTEX_INPUT vec4 instanceColour;

VERTEX_OUTPUT vec2 uv;
VERTEX_OUTPUT vec4 colour;

void main()
{
	// unpack inputs
	#define transformationMatrix instanceModelMatrix
	vec2 tuv_offset = instancePackedValues.xy;
	vec2 tuv_scale = instancePackedValues.zw;

	gl_Position = ProjectionMatrix * transformationMatrix * vertex;
	uv = tuv_scale * vertex.xy + tuv_offset;

	// pack outputs for fragment shader
	colour = instanceColour;
}
