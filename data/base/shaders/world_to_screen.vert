// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec2 vertexPos;
in vec2 vertexTexCoords;
#else
attribute vec2 vertexPos;
attribute vec2 vertexTexCoords;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec2 texCoords;
#else
varying vec2 texCoords;
#endif

void main()
{
	texCoords = vertexTexCoords;
	gl_Position = vec4(vertexPos.x, vertexPos.y, 0.f, 1.f);
}
