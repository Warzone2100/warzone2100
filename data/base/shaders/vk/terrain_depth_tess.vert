#version 450

// Passthrough of the tile-corner control points (positions only) for the
// terrain depth passes.

layout(location = 0) in vec4 vertex;

void main()
{
	gl_Position = vertex;
}
