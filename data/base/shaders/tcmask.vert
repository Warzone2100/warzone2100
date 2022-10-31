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
uniform mat4 ModelViewMatrix;
uniform mat4 NormalMatrix;
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
#else
varying float vertexDistance;
varying vec3 normal, lightDir, halfVec;
varying vec2 texCoord;
#endif

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoordAndTexAnim.xy;
	int framesPerLine = int(1.f / min(vertexTexCoordAndTexAnim.z, 1.f)); // texAnim.x
	int frame = int(animFrameNumber);
	float uFrame = float(intMod(frame, framesPerLine)) * vertexTexCoordAndTexAnim.z; // texAnim.x
	float vFrame = float(frame / framesPerLine) * vertexTexCoordAndTexAnim.w; // texAnim.y
	texCoord = vec2(texCoord.x + uFrame, texCoord.y + vFrame);

	// Lighting we pass to the fragment shader
	vec4 viewVertex = ModelViewMatrix * vec4(vertex.xyz, -vertex.w); // FIXME
	vec3 eyeVec = normalize(-viewVertex.xyz);
	vec3 n = normalize((NormalMatrix * vec4(vertexNormal, 0.0)).xyz);
	lightDir = normalize(lightPosition.xyz);

	if (hasTangents != 0)
	{
		// Building the matrix Eye Space -> Tangent Space with handness
		vec3 t = normalize((NormalMatrix * vertexTangent).xyz);
		vec3 b = cross (n, t) * vertexTangent.w;
		mat3 TangentSpaceMatrix = mat3(t, n, b);

		// Transform light and eye direction vectors by tangent basis
		lightDir *= TangentSpaceMatrix;
		eyeVec *= TangentSpaceMatrix;
	}

	normal = n;
	halfVec = lightDir + eyeVec;

	// Implement building stretching to accommodate terrain
	vec4 position = vertex;
	if (vertex.y <= 0.0) // use vertex here directly to help shader compiler optimization
	{
		position.y -= stretch;
	}

	// Translate every vertex according to the Model View and Projection Matrix
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	vec4 gposition = ModelViewProjectionMatrix * position;
	gl_Position = gposition;

	// Remember vertex distance
	vertexDistance = gposition.z;
}
