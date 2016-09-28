#version 130

uniform vec2 from;
uniform vec2 to;
uniform mat4 ModelViewProjectionMatrix;

vec4 getPos(int id)
{
	if (id == 0) return vec4(from, 0., 1.);
	if (id == 1) return vec4(to, 0., 1.);
}

void main()
{
	gl_Position = ModelViewProjectionMatrix * getPos(gl_VertexID);
}
