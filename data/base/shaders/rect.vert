#version 120

// gl_VertexID seems to not be supported on 120, despite documentation to the contrary.

// Old comment:
// This shader uses vertex id to generate
// vertex position. This allows to save
// a vertex buffer binding and thus
// simplifies C++ code.

uniform mat4 transformationMatrix;
uniform vec2 tuv_offset;
uniform vec2 tuv_scale;

attribute vec4 rect;

varying vec2 uv;


void main()
{
	gl_Position = transformationMatrix * rect;
	uv = tuv_scale * rect.xy + tuv_offset;
}
