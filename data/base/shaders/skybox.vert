// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform mat4 posMatrix;

uniform int fog_enabled;
uniform vec4 fog_color;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 vertex;
in vec2 vertexTexCoord;
in vec4 vertexColor;
#else
attribute vec4 vertex;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec2 uv;
out vec4 fog;
#else
varying vec2 uv;
varying vec4 fog;
#endif

void main()
{
	// Pass texture coordinates to fragment shader
	uv = vertexTexCoord;

	gl_Position = posMatrix * vertex;

	if(fog_enabled > 0)
	{
		fog =
		vertex.y < -0.7 ? vec4(0, 0, 0, 1) :
		vertex.y < 0.5 ? fog_color :
		vec4(fog_color.xyz, 0);
	}
}
