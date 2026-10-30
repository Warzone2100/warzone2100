// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core, and GLSL ES 3.00.)

// Adds the bloom onto the scene (the color mask keeps the scene's alpha)

layout(std140) uniform cbuffer {
	vec4 uvScaleClamp; // of the largest bloom level
	vec4 params;       // xy = 1 / its texture size, z = tent radius, w = intensity
	vec4 params2;      // x = 1 / number of levels summed into it
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
	vec3 bloom = bloomUpsampleTent(uv, params.xy, params.z) * params2.x;
	FragColor = vec4(bloom * params.w, 1.0);
}
