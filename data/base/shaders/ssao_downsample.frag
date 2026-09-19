// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	vec4 uvScaleClamp;
};
uniform sampler2D occlusionTexture;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex, uv) texture2D(tex, uv)
#endif

#ifdef NEWGL
in vec2 texCoords;
#else
varying vec2 texCoords;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	float ao = texture(occlusionTexture, uv).r;

	#ifdef NEWGL
	FragColor = vec4(vec3(ao), 1.0);
	#else
	gl_FragColor = vec4(vec3(ao), 1.0);
	#endif
}
