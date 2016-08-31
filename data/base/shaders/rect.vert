#version 130

// This shader uses vertex id to generate
// vertex position. This allows to save
// a vertex buffer binding and thus
// simplifies C++ code.

uniform mat4 transformationMatrix;
uniform vec2 tuv_offset;
uniform vec2 tuv_scale;

out vec2 uv;

vec4 getPos(int id)
{
	if (id == 0) return vec4(0, 1, 0, 1);
	if (id == 1) return vec4(0, 0, 0, 1);
	if (id == 2) return vec4(1, 1, 0, 1);
	if (id == 3) return vec4(1, 0, 0, 1);
}

vec2 getTC(int id)
{
	if (id == 0) return vec2(0., 1.);
	if (id == 1) return vec2(0., 0.);
	if (id == 2) return vec2(1., 1.);
	if (id == 3) return vec2(1., 0.);
}

void main()
{
	gl_Position = transformationMatrix * getPos(gl_VertexID);
	uv = tuv_scale * getTC(gl_VertexID) + tuv_offset;
}
