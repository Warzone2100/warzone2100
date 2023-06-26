#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	float gamma;
};

layout(location = 0) in vec2 vertexPos;

layout(location = 0) out vec2 texCoords;

void main()
{
	gl_Position = vec4(vertexPos.x, vertexPos.y, 0.0, 1.0);
	texCoords = 0.5 * gl_Position.xy + vec2(0.5);
}
