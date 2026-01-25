// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 ModelViewProjectionMatrix;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
//in vec4 vertexColor;
#else
attribute vec4 vertex;
//attribute vec4 vertexColor;
#endif

void main()
{
	vec4 position = ModelViewProjectionMatrix * vertex;
	position.z += 0.01f;
	gl_Position = position;
}
