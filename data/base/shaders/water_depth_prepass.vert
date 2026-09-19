// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ViewMatrix;
};

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
out vec3 viewNormal;
#else
attribute vec4 vertex;
varying vec3 viewNormal;
#endif

void main()
{
	gl_Position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.0);
	viewNormal = mat3(ViewMatrix) * vec3(0.0, 1.0, 0.0);
}
