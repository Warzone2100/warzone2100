// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// Texture arrays are supported in:
// 1. core OpenGL 3.0+ (and earlier with EXT_texture_array)
#if (!defined(GL_ES) && (__VERSION__ < 130))
#extension GL_EXT_texture_array : require // only enable this on OpenGL < 3.0
#endif
// 2. OpenGL ES 3.0+
#if (defined(GL_ES) && (__VERSION__ < 300))
#error "Unsupported version of GLES"
#endif

uniform ivec4 swizzle;
uniform vec4 color;
uniform int layer;
uniform sampler2DArray theTexture;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#define texture2DArray(tex,coord,bias) texture(tex,coord,bias)
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#endif

#ifdef NEWGL
in vec2 uv;
#else
varying vec2 uv;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

float getVecComponent(vec4 v, int c)
{
	float f = (c == 0) ? v.x :
		(c == 1) ? v.y :
		(c == 2) ? v.z : v.w;
	return f;
}

void main()
{
	vec4 texColour = texture2DArray(theTexture, vec3(uv, layer), 0.f);

	vec4 fragColour = vec4(getVecComponent(texColour, swizzle.x), getVecComponent(texColour, swizzle.y), getVecComponent(texColour, swizzle.z), getVecComponent(texColour, swizzle.w));

	#ifdef NEWGL
	FragColor = fragColour * color;
	#else
	gl_FragColor = fragColour * color;
	#endif
}
