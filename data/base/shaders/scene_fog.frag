// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

#include "view_position.glsl"
#include "distance_fog.glsl"

layout(std140) uniform cbuffer {
	vec4 fogColor;
	mat4 invProjectionMatrix;
	vec4 uvScaleClamp;
	float fogBegin;
	float fogEnd;
	float padding0;
	float padding1;
};
uniform sampler2D sceneTexture;
uniform sampler2D prepassDepth;

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

const float SKY_DEPTH_THRESHOLD = 0.9999;

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	vec3 lit = texture(sceneTexture, uv).rgb;
	vec3 result = lit;
	float depth = texture(prepassDepth, uv).r;
	if (depth < SKY_DEPTH_THRESHOLD)
	{
		float viewDist = length(wzGetViewPosition(uv, depth, invProjectionMatrix));
		result = mix(lit, fogColor.rgb, wzDistanceFogAmount(viewDist, fogBegin, fogEnd));
	}
	// Sky / no prepass geometry: skybox already applied its own fog in ScenePass.

	#ifdef NEWGL
	FragColor = vec4(result, 1.0);
	#else
	gl_FragColor = vec4(result, 1.0);
	#endif
}
