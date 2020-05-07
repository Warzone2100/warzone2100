// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 posMatrix;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
in vec2 vertexTexCoord;
in vec4 vertexColor;
#else
attribute vec4 vertex;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec2 uv;
out vec4 vColour;
#else
varying vec2 uv;
varying vec4 vColour;
#endif

void main()
{
	// Pass texture coordinates to fragment shader
	uv = vertexTexCoord;

	gl_Position = posMatrix * vertex;
	vColour = vertexColor;
}
