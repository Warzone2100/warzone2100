// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

#if !defined(NEWGL) && defined(GL_EXT_gpu_shader4)
#extension GL_EXT_gpu_shader4 : enable
#endif

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ModelUVLightmapMatrix;

uniform int hasTangents; // whether tangents were calculated for model
uniform vec4 lightPosition; // in view space
uniform vec4 cameraPos; // in model space

#if defined(NEWGL) || defined(GL_EXT_gpu_shader4)
#define intMod(a, b) a % b
#else
#define intMod(a, b) floor((a - floor((a + 0.5) / b) * b) + 0.5)
#endif

#ifdef NEWGL
#define VERTEX_INPUT in
#define VERTEX_OUTPUT out
#else
#define VERTEX_INPUT attribute
#define VERTEX_OUTPUT varying
#endif

VERTEX_INPUT vec4 vertex;
VERTEX_INPUT vec3 vertexNormal;
VERTEX_INPUT vec4 vertexTexCoordAndTexAnim;
VERTEX_INPUT vec4 vertexTangent;
VERTEX_INPUT mat4 instanceModelMatrix;
VERTEX_INPUT vec4 instancePackedValues; // shaderStretch_ecmState_alphaTest_animFrameNumber
VERTEX_INPUT vec4 instanceColour;
VERTEX_INPUT vec4 instanceTeamColour;

VERTEX_OUTPUT vec3 normal, lightDir, halfVec;
VERTEX_OUTPUT mat3 NormalMatrix; // local to model space for directional vectors
VERTEX_OUTPUT mat3 TangentSpaceMatrix;
VERTEX_OUTPUT vec4 colour;
VERTEX_OUTPUT vec4 teamcolour;
VERTEX_OUTPUT vec4 packed_ecmState_alphaTest_texCoord;
VERTEX_OUTPUT vec3 uvLightmap; // uvLightmap in .xy, heightAboveTerrain in .z

// for Shadows
VERTEX_OUTPUT vec3 posModelSpace;
VERTEX_OUTPUT vec3 posViewSpace;

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
	float uFrame = float(intMod(frame, framesPerLine)) * vertexTexCoordAndTexAnim.z; // texAnim.x
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

	// pack outputs for fragment shader
	colour = instanceColour;
	teamcolour = instanceTeamColour;
	packed_ecmState_alphaTest_texCoord = vec4(ecmState, alphaTest, texCoord);
}
