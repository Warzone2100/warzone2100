// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform vec4 cameraPos; // in model space
uniform vec4 sunPos;
uniform int viewportWidth;
uniform int viewportHeight;

#ifdef NEWGL
#define VERTEX_INPUT in
#define VERTEX_OUTPUT out
#else
#define VERTEX_INPUT attribute
#define VERTEX_OUTPUT varying
#endif

VERTEX_INPUT vec2 vertexPos;

VERTEX_OUTPUT vec2 texCoords;

void main()
{
	gl_Position = vec4(vertexPos.x, vertexPos.y, 0.0, 1.0);
	texCoords = 0.5 * gl_Position.xy + vec2(0.5);
}
