// Version directive is set by Warzone when loading the shader.

layout(std140) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelViewMatrix;
	vec4 color;
	vec4 fogColor;
	vec4 fogRange;
};

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
out vec3 posViewSpace;
#else
attribute vec4 vertex;
varying vec3 posViewSpace;
#endif

void main()
{
	gl_Position = ModelViewProjectionMatrix * vertex;
	posViewSpace = (ModelViewMatrix * vertex).xyz;
}
