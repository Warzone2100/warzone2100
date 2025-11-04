// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#endif

uniform sampler2D Texture;
uniform sampler2D depthTexture;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform vec4 cameraPos; // in model space
uniform vec4 sunPos;
uniform int viewportWidth;
uniform int viewportHeight;
uniform float gamma;

#ifdef NEWGL
#define FRAGMENT_INPUT in
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#define FRAGMENT_INPUT varying
#endif

FRAGMENT_INPUT vec2 texCoords;

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	vec3 texColour = texture(Texture, texCoords).rgb;

	#ifdef NEWGL
	FragColor = vec4(texColour, 1.0);
	#else
	gl_FragColor = vec4(texColour, 1.0);
	#endif
}
