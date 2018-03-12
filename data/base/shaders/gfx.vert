uniform mat4 posMatrix;

#if __VERSION__ >= 130
in vec4 vertex;
in vec2 vertexTexCoord;
in vec4 vertexColor;
#else
attribute vec4 vertex;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;
#endif

#if __VERSION__ >= 130
out vec2 uv;
out vec4 vColour;
#else
varying vec2 uv;
varying vec4 vColour;
#endif

void main()
{
	// Pass texture coordinates to fragment shader
	uv = vertexTexCoord;

	gl_Position = posMatrix * vertex;
	vColour = vertexColor;
}
