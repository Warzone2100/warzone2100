#version 450

layout(quads, equal_spacing, ccw) in;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	vec4 color;
	float tessLevel;
};

layout(location = 0) out vec2 tessUV;

void main()
{
	vec2 uv = gl_TessCoord.xy;
	vec4 bottom = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, uv.x);
	vec4 top = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, uv.x);
	vec4 pos = mix(bottom, top, uv.y);
	// bulge the patch interior so real tessellation is visually unmistakable
	// (edges/corners stay fixed while interior vertices shift, bending the fragment grid lines)
	float bulge = sin(uv.x * 3.14159265) * sin(uv.y * 3.14159265);
	pos.xy += vec2(0.05, 0.08) * bulge;
	tessUV = uv;
	gl_Position = transformationMatrix * pos;
	// Vulkan clip-space fixup (this is the final position-producing stage)
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
