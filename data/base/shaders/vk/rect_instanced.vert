#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ProjectionMatrix;
};

layout(location = 0) in vec4 vertex;
layout(location = 5) in mat4 instanceModelMatrix;
layout(location = 9) in vec4 instancePackedValues; // vec2 (offset), vec2 (scale)
layout(location = 10) in vec4 instanceColour;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 colour;

void main()
{
	// unpack inputs
	#define transformationMatrix instanceModelMatrix
	vec2 tuv_offset = instancePackedValues.xy;
	vec2 tuv_scale = instancePackedValues.zw;

	gl_Position = ProjectionMatrix * transformationMatrix * vertex;
	uv = tuv_scale * vertex.xy + tuv_offset;
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;

	// pack outputs for fragment shader
	colour = instanceColour;
}
