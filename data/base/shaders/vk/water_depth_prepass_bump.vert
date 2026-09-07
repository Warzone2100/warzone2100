#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ViewMatrix;
	float timeSec;
	float mipLoadBias;
	float pad0;
	float pad1;
};

layout(location = 0) in vec4 vertex;

layout(location = 0) out vec4 uv1_uv2;

void main()
{
	gl_Position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.0);
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	vec2 uv1 = vec2(vertex.x/3.f/128.f, -vertex.z/3.f/128.f + timeSec/45.f);
	vec2 uv2 = vec2(vertex.x/4.f/128.f, -vertex.z/4.f/128.f - timeSec/60.f);
	uv1_uv2 = vec4(uv1.x, uv1.y, uv2.x, uv2.y);
}
