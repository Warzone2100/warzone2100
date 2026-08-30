// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	vec4 uvScaleClamp;
	float tonemapKnee;
	float encodeOutput;
	float padding0;
	float padding1;
};
uniform sampler2D sceneTexture;

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

#include "srgb.glsl"

// Leaves values below the knee unchanged and rolls values above it off smoothly toward 1
vec3 tonemapShoulder(vec3 color, float knee)
{
	vec3 excess = max(color - vec3(knee), vec3(0.0));
	float headroom = max(1.0 - knee, 1e-4);
	vec3 shoulder = headroom * (vec3(1.0) - exp(-excess / headroom));
	return min(color, vec3(knee)) + shoulder;
}

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	vec4 scene = texture(sceneTexture, uv);

	vec3 result = tonemapShoulder(scene.rgb, tonemapKnee);
	if (encodeOutput > 0.5)
	{
		// Linear lighting is on, so encode the scene for display
		result = wzLinearToSrgb(result);
	}

#ifdef NEWGL
	FragColor = vec4(result, scene.a);
#else
	gl_FragColor = vec4(result, scene.a);
#endif
}
