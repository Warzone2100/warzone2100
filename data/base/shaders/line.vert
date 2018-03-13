// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform vec2 from;
uniform vec2 to;
uniform mat4 ModelViewProjectionMatrix;

#if __VERSION__ >= 130
in vec4 vertex;
#else
attribute vec4 vertex;
#endif


void main()
{
	vec4 pos = vec4(from + (to - from)*vertex.y, 0.0, 1.0);
	gl_Position = ModelViewProjectionMatrix * pos;
}
