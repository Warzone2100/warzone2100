// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

layout(std140) uniform cbuffer {
	mat4 transformationMatrix;
	mat4 uvTransformMatrix;
	ivec4 swizzle;
	vec4 color;
};

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
	vec4 calculatedCoord = uvTransformMatrix * vec4(vertex.xy, 1.f, 1.f);
	uv = calculatedCoord.xy;
}
