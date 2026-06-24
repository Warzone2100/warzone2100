#version 450
//#pragma debug(on)

#include "tcmask_instanced.glsl"

layout(location = 0) in vec4 vertex;
layout(location = 3) in vec3 vertexNormal;
layout(location = 1) in vec4 vertexTexCoordAndTexAnim;
layout(location = 4) in vec4 vertexTangent;
layout(location = 5) in mat4 instanceModelMatrix;
layout(location = 9) in vec4 instancePackedValues; // shaderStretch_ecmState_alphaTest_animFrameNumber
layout(location = 10) in vec4 instanceColour;
layout(location = 11) in vec4 instanceTeamColour;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec3 lightDir;
layout(location = 2) out vec3 halfVec;
layout(location = 3) out mat3 NormalMatrix; // local to model space for directional vectors
layout(location = 6) out mat3 TangentSpaceMatrix;
layout(location = 9) out vec4 colour;
layout(location = 10) out vec4 teamcolour;
layout(location = 11) out vec4 packed_ecmState_alphaTest_texCoord;
layout(location = 12) out vec3 uvLightmap; // uvLightmap in .xy, heightAboveTerrain in .z
// For Shadows
layout(location = 13) out vec3 posModelSpace;
layout(location = 14) out vec3 posViewSpace;

float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
	// unpack inputs
	float stretch = instancePackedValues.x;
	float ecmState = instancePackedValues.y;
	float alphaTest = instancePackedValues.z;
	float animFrameNumber = instancePackedValues.w;

	// Pass texture coordinates to fragment shader
	vec2 texCoord = vertexTexCoordAndTexAnim.xy;
	int framesPerLine = int(1.f / min(vertexTexCoordAndTexAnim.z, 1.f)); // texAnim.x
	int frame = int(animFrameNumber);
	float uFrame = float(frame % framesPerLine) * vertexTexCoordAndTexAnim.z; // texAnim.x
	float vFrame = float(frame / framesPerLine) * vertexTexCoordAndTexAnim.w; // texAnim.y
	texCoord = vec2(texCoord.x + uFrame, texCoord.y + vFrame);

	mat4 ModelVeiwMatrix = ViewMatrix * instanceModelMatrix;
	NormalMatrix = mat3(transpose(inverse(instanceModelMatrix)));

	// transform face normals of classic models to World Space
	normal = -normalize(NormalMatrix * vertexNormal);

	if (hasTangents != 0)
	{
		// Building the World Space <-> Tangent Space matrix with handness w to support uv mirroring
		normal = normalize(NormalMatrix * vertexNormal);
		vec3 t = normalize(NormalMatrix * vertexTangent.xyz);
		vec3 b = cross (normal, t) * vertexTangent.w;
		TangentSpaceMatrix = mat3(t, normal, b);
	}

	// Lighting
	posViewSpace = vec3(ModelVeiwMatrix * vertex);
	posModelSpace = vec3(instanceModelMatrix * vertex);
	vec3 cameraVec = normalize(cameraPos.xyz - posModelSpace.xyz);
	lightDir = -normalize(mat3(inverse(ViewMatrix)) * lightPosition.xyz); //to-do: pass Sun pos in world space
	halfVec = lightDir + cameraVec;

	vec3 localPosition = vertex.xyz;

	// Implement building stretching to accommodate terrain
	if (vertex.y <= 0.0) // use vertex here directly to help shader compiler optimization
	{
		// NOTE: 'stretch' may be:
		//	- if positive: building stretching
		//	- if negative: the height above the terrain of the model intance overall
		localPosition.y -= (stretch * when_gt(stretch, 0.f));
	}

	float heightAboveTerrain = abs(clamp(sign(stretch), -1.f, 0.f)) * abs(stretch);
	uvLightmap = vec3((ModelUVLightmapMatrix * vec4(posModelSpace, 1.0)).xy, localPosition.y + heightAboveTerrain);

	// Translate every vertex according to the Model View and Projection Matrix
	vec4 gposition = ProjectionMatrix * ModelVeiwMatrix * vec4(localPosition, vertex.w);
	gl_Position = gposition;

	gl_Position.y *= -1.;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;

	// pack outputs for fragment shader
	colour = instanceColour;
	teamcolour = instanceTeamColour;
	packed_ecmState_alphaTest_texCoord = vec4(ecmState, alphaTest, texCoord.x, texCoord.y);
}
