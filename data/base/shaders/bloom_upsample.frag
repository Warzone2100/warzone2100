// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core, and GLSL ES 3.00.)

// One step up the bloom chain: adds the tent-filtered smaller level onto this one

layout(std140) uniform cbuffer {
	vec4 uvScaleClamp; // of the smaller level being read
	vec4 params;       // xy = 1 / its texture size, z = tent radius in its texels
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
	FragColor = vec4(bloomUpsampleTent(uv, params.xy, params.z), 1.0);
}
