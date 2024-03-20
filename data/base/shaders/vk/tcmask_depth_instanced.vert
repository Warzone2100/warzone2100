#version 450
//#pragma debug(on)

layout(std140, set = 0, binding = 0) uniform globaluniforms
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
};

layout(location = 0) in vec4 vertex;
layout(location = 3) in vec3 vertexNormal;
//layout(location = 1) in vec4 vertexTexCoordAndTexAnim;
//layout(location = 4) in vec4 vertexTangent;
layout(location = 5) in mat4 instanceModelMatrix;
layout(location = 9) in vec4 instancePackedValues; // shaderStretch_ecmState_alphaTest_animFrameNumber
layout(location = 10) in vec4 instanceColour;
layout(location = 11) in vec4 instanceTeamColour;

float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
	// unpack inputs
	mat4 ModelViewMatrix = ViewMatrix * instanceModelMatrix;
	float stretch = instancePackedValues.x;

	// Implement building stretching to accommodate terrain
	vec4 position = vertex;
	if (vertex.y <= 0.0) // use vertex here directly to help shader compiler optimization
	{
		// NOTE: 'stretch' may be:
		//	- if positive: building stretching
		//	- if negative: the height above the terrain of the model intance overall
		position.y -= (stretch * when_gt(stretch, 0.f));
	}

	// Translate every vertex according to the Model View and Projection Matrix
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	vec4 gposition = ModelViewProjectionMatrix * position;
	gl_Position = gposition;

	// Remember vertex distance
//	float vertexDistance = gposition.z;
//	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
