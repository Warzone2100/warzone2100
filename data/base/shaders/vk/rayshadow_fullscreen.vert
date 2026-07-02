#version 460

// Fullscreen triangle (no vertex buffer required)

layout(location = 0) out vec2 texCoords;

void main()
{
	vec2 pos = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
	texCoords = pos;
	gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
