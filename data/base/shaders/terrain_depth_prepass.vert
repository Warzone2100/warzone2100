// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

layout(std140) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ViewMatrix;
};

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
in vec3 vertexNormal;
out vec3 viewNormal;
#else
attribute vec4 vertex;
attribute vec3 vertexNormal;
varying vec3 viewNormal;
#endif

void main()
{
	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	viewNormal = mat3(ViewMatrix) * vertexNormal;
}
