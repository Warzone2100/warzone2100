// gl_VertexID seems to not be supported on 120, despite documentation to the contrary.

// Old comment:
// This shader uses vertex id to generate
// vertex position. This allows to save
// a vertex buffer binding and thus
// simplifies C++ code.

uniform mat4 transformationMatrix;
uniform vec2 tuv_offset;
uniform vec2 tuv_scale;

#if __VERSION__ >= 130
in vec4 vertex;
#else
attribute vec4 vertex;
#endif

#if __VERSION__ >= 130
out vec2 uv;
#else
varying vec2 uv;
#endif


void main()
{
	gl_Position = transformationMatrix * vertex;
	uv = tuv_scale * vertex.xy + tuv_offset;
}
