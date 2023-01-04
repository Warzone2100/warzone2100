// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
in vec2 vertexTexCoord;
#else
attribute vec4 vertex;
attribute vec2 vertexTexCoord;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec2 texCoord;
out float vertexDistance;
#else
varying vec2 texCoord;
varying float vertexDistance;
#endif

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoord;

	// Translate every vertex according to the Model View and Projection Matrix
	mat4 ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	vec4 gposition = ModelViewProjectionMatrix * vertex;
	gl_Position = gposition;

	// Remember vertex distance
	vertexDistance = gposition.z;
}
