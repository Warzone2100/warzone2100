#version 450

layout(location = 0) in vec2 vertex;

// WZ_CLIP_NOT_FINAL: outputs tessellation control points
void main()
{
	// control points pass through to the tessellation stages. The transform happens in the TES.
	gl_Position = vec4(vertex, 0.0, 1.0);
}
