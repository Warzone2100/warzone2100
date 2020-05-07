// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// gl_VertexID seems to not be supported on 120, despite documentation to the contrary.

// Old comment:
// This shader uses vertex id to generate
// vertex position. This allows to save
// a vertex buffer binding and thus
// simplifies C++ code.

uniform mat4 transformationMatrix;
uniform vec2 tuv_offset;
uniform vec2 tuv_scale;

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
	uv = tuv_scale * vertex.xy + tuv_offset;
}
