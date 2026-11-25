#version 450

layout(location = 0) in vec4 vertex;

// WZ_CLIP_NOT_FINAL: outputs tessellation control points
void main()
{
	gl_Position = vertex;
}
