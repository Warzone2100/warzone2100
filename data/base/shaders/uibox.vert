// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 transformationMatrix;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
#else
attribute vec4 vertex;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec2 uv;
#else
varying vec2 uv;
#endif

void main()
{
	gl_Position = transformationMatrix * vertex;
	// The shared rect buffer is a unit quad - vertex.xy doubles as 0..1 uv.
	uv = vertex.xy;
}
