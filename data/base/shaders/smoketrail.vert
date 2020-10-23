

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec4 c;
in vec2 texCoord;
in float t;
#else
attribute vec4 c;
attribute vec2 texCoord;
attribute float t;
#endif

uniform mat4 ModelViewProjectionMatrix;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec2 TexCoord;
out float T;
#else
varying vec2 TexCoord;
varying float T;
#endif

void main(void) {
    gl_Position = ModelViewProjectionMatrix * c;
    TexCoord = texCoord;
    T = t;
}