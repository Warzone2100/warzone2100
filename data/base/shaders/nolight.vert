// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

uniform mat4 ModelViewProjectionMatrix;

#if __VERSION__ >= 130
in vec4 vertex;
in vec2 vertexTexCoord;
#else
attribute vec4 vertex;
attribute vec2 vertexTexCoord;
#endif

#if __VERSION__ >= 130
out vec2 texCoord;
#else
varying vec2 texCoord;
#endif

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoord;

	// Translate every vertex according to the Model, View and Projection matrices
	gl_Position = ModelViewProjectionMatrix * vertex;
}
