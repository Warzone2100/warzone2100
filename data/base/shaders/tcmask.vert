// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

uniform float stretch;
uniform mat4 ModelViewMatrix;
uniform mat4 ModelViewProjectionMatrix;
uniform mat4 NormalMatrix;
uniform int hasTangents; // whether tangents were calculated for model

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
in vec4 vertexTangent;
#else
attribute vec4 vertex;
attribute vec3 vertexNormal;
attribute vec2 vertexTexCoord;
attribute vec4 vertexTangent;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out float vertexDistance;
out vec3 normal;
out vec2 texCoord;
out vec3 vVertex;
out vec3 eyeVec;
out mat3 TangentSpaceMatrix;
#else
varying float vertexDistance;
varying vec3 normal;
varying vec2 texCoord;
varying vec3 vVertex;
varying vec3 eyeVec;
varying mat3 TangentSpaceMatrix;
#endif

void main()
{
	vVertex = normalize((ModelViewMatrix * vertex).xyz);
	vec4 position = vertex;

	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoord;

	// Lighting -- we pass these to the fragment shader
	vec3 n = normalize((NormalMatrix * vec4(vertexNormal, 0.0)).xyz);
	vec3 e = -vVertex;

	if (hasTangents != 0)
	{
		// Building the matrix Eye Space -> Tangent Space with handness
		vec3 t = normalize((NormalMatrix * vertexTangent).xyz);
		vec3 b = cross (n, t) * vertexTangent.w;
		TangentSpaceMatrix = mat3(t, n, b);

		// Transform calculated normals for vanilla models by tangent basis
		n *= TangentSpaceMatrix;

		// Transform light and eye direction vectors by tangent basis
		e *= TangentSpaceMatrix;
	}

	normal = n;
	eyeVec = e;

	// Implement building stretching to accommodate terrain
	if (vertex.y <= 0.0) // use vertex here directly to help shader compiler optimization
	{
		position.y -= stretch;
	}

	// Translate every vertex according to the Model View and Projection Matrix
	vec4 gposition = ModelViewProjectionMatrix * position;
	gl_Position = gposition;

	// Remember vertex distance
	vertexDistance = gposition.z;
}
