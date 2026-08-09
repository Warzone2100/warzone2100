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
uniform mat4 ModelMatrix;
uniform mat4 NormalMatrix;
uniform vec4 cameraPos; // in world space
uniform int hasTangents; // whether tangents were calculated for model
uniform vec4 lightPosition;
uniform float stretch;
uniform float animFrameNumber;

#if defined(NEWGL) || defined(GL_EXT_gpu_shader4)
#define intMod(a, b) a % b
#else
#define intMod(a, b) floor((a - floor((a + 0.5) / b) * b) + 0.5)
#endif

#ifdef NEWGL
in vec4 vertex;
in vec3 vertexNormal;
in vec4 vertexTexCoordAndTexAnim;
in vec4 vertexTangent;
#else
attribute vec4 vertex;
attribute vec3 vertexNormal;
attribute vec4 vertexTexCoordAndTexAnim;
attribute vec4 vertexTangent;
#endif

#ifdef NEWGL
out float vertexDistance;
out vec3 normal, lightDir, halfVec;
out vec2 texCoord;
out mat3 TangentSpaceMatrix;
#else
varying float vertexDistance;
varying vec3 normal, lightDir, halfVec;
varying vec2 texCoord;
varying mat3 TangentSpaceMatrix;
#endif

float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoordAndTexAnim.xy;
	int framesPerLine = int(1.f / min(vertexTexCoordAndTexAnim.z, 1.f)); // texAnim.x
	int frame = int(animFrameNumber);
	float uFrame = float(intMod(frame, framesPerLine)) * vertexTexCoordAndTexAnim.z; // texAnim.x
	float vFrame = float(frame / framesPerLine) * vertexTexCoordAndTexAnim.w; // texAnim.y
	texCoord = vec2(texCoord.x + uFrame, texCoord.y + vFrame);

	// Lighting, all in WORLD space
	vec3 posWorld = (ModelMatrix * vertex).xyz;
	vec3 cameraVec = normalize(cameraPos.xyz - posWorld);

	// Classic models are lit with the normal negated, which cancels against the
	// negated light below.
	normal = -normalize((NormalMatrix * vec4(vertexNormal, 0.0)).xyz);
	lightDir = -normalize(mat3(inverse(ViewMatrix)) * lightPosition.xyz);

	if (hasTangents != 0)
	{
		// ...but NOT negated when it is the shading normal of a tangent basis.
		normal = normalize((NormalMatrix * vec4(vertexNormal, 0.0)).xyz);

		// Building the World Space -> Tangent Space matrix with handness w to
		// support uv mirroring. The fragment shader applies it.
		vec3 t = normalize((NormalMatrix * vertexTangent).xyz);
		vec3 b = cross (normal, t) * vertexTangent.w;
		TangentSpaceMatrix = mat3(t, b, normal); // conventional (T, B, N)
	}

	halfVec = lightDir + cameraVec;

	// Implement building stretching to accommodate terrain
	vec4 position = vertex;
	if (vertex.y <= 0.0) // use vertex here directly to help shader compiler optimization
	{
		// NOTE: 'stretch' may be:
		//	- if positive: building stretching
		//	- if negative: the height above the terrain of the model instance overall
		position.y -= (stretch * when_gt(stretch, 0.f));
	}

	// Translate every vertex according to the Model View and Projection Matrix
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix;
	vec4 gposition = ModelViewProjectionMatrix * position;
	gl_Position = gposition;

	// Remember vertex distance
	vertexDistance = gposition.z;
}
