#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 transformationMatrix;
	mat4 uvTransformMatrix;
	ivec4 swizzle;
	vec4 color;
	int layer;
};

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 FragColor;

float getVecComponent(vec4 v, int c)
{
	float f = (c == 0) ? v.x :
		(c == 1) ? v.y :
		(c == 2) ? v.z : v.w;
	return f;
}

void main()
{
	vec4 texColour = texture(tex, uv, 0.f);

	vec4 fragColour = vec4(getVecComponent(texColour, swizzle.x), getVecComponent(texColour, swizzle.y), getVecComponent(texColour, swizzle.z), getVecComponent(texColour, swizzle.w));

	FragColor = fragColour * color;
}
