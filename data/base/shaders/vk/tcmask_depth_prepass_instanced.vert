#version 450

layout(std140, set = 0, binding = 0) uniform globaluniforms
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
};

layout(location = 0) in vec4 vertex;
layout(location = 3) in vec3 vertexNormal;
layout(location = 5) in mat4 instanceModelMatrix;
layout(location = 9) in vec4 instancePackedValues;
layout(location = 10) in vec4 instanceColour;
layout(location = 11) in vec4 instanceTeamColour;

layout(location = 0) out vec3 viewNormal;

float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
	mat4 ModelViewMatrix = ViewMatrix * instanceModelMatrix;
	float stretch = instancePackedValues.x;

	vec4 position = vertex;
	if (vertex.y <= 0.0)
	{
		position.y -= (stretch * when_gt(stretch, 0.f));
	}

	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	gl_Position = ModelViewProjectionMatrix * position;
	// Match scene mesh / terrain SSAO depth clip conventions on Vulkan.
	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;

	viewNormal = mat3(ModelViewMatrix) * vertexNormal;
}
