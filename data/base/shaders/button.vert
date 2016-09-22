#version 120
#pragma debug(on)

uniform mat4 ModelViewProjectionMatrix;

attribute vec4 vertex;
attribute vec4 vertexTexCoord;

varying vec2 texCoord;

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = (gl_TextureMatrix[0] * vertexTexCoord).xy;

	// Translate every vertex according to the Model, View and Projection matrices
	gl_Position = ModelViewProjectionMatrix * vertex;
}
