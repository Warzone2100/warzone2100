// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ViewMatrix;
	float timeSec;
	float mipLoadBias;
	float pad0;
	float pad1;
};

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
out vec4 uv1_uv2;
#else
attribute vec4 vertex;
varying vec4 uv1_uv2;
#endif

void main()
{
	gl_Position = ModelViewProjectionMatrix * vec4(vertex.xyz, 1.0);
	vec2 uv1 = vec2(vertex.x/3.f/128.f, -vertex.z/3.f/128.f + timeSec/45.f);
	vec2 uv2 = vec2(vertex.x/4.f/128.f, -vertex.z/4.f/128.f - timeSec/60.f);
	uv1_uv2 = vec4(uv1.x, uv1.y, uv2.x, uv2.y);
}
