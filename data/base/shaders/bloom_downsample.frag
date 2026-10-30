// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core, and GLSL ES 3.00.)

// One step down the bloom chain, to half the source size

layout(std140) uniform cbuffer {
	vec4 uvScaleClamp; // of the source
	vec4 params;       // xy = 1 / source size, z = 1 for the 9-tap fit, w = threshold (first step only)
};

uniform sampler2D sourceTexture;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex, uv) texture2D(tex, uv)
#endif

#ifdef NEWGL
in vec2 texCoords;
out vec4 FragColor;
#else
varying vec2 texCoords;
#define FragColor gl_FragColor
#endif

#include "bloom_filters.glsl"

void main()
{
	vec2 uv = texCoords * uvScaleClamp.xy;
	vec3 c = (params.z > 0.5) ? bloomDownsample9(uv, params.xy) : bloomDownsample13(uv, params.xy, params.w);
	FragColor = vec4(c, 1.0);
}
