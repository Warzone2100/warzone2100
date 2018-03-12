uniform mat4 ModelViewProjectionMatrix;

#if __VERSION__ >= 130
in vec4 vertex;
#else
attribute vec4 vertex;
#endif

void main()
{
	gl_Position = ModelViewProjectionMatrix * vertex;
}
