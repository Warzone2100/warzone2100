#version 450

layout(vertices = 4) out;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	vec4 color;
	float tessLevel;
};

void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelOuter[0] = tessLevel;
		gl_TessLevelOuter[1] = tessLevel;
		gl_TessLevelOuter[2] = tessLevel;
		gl_TessLevelOuter[3] = tessLevel;
		gl_TessLevelInner[0] = tessLevel;
		gl_TessLevelInner[1] = tessLevel;
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
