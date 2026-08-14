// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

#include "ssao_view_common.glsl"

layout(std140) uniform cbuffer {
	float ssaoIntensity;
	float fogStart;
	float fogEnd;
	int fogEnabled;
	vec4 fogColor;
	mat4 invProjectionMatrix;
	vec4 uvScaleClamp;
};
uniform sampler2D sceneTexture;
uniform sampler2D ssaoTexture;
uniform sampler2D prepassNormals;
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
	vec3 scene = texture(sceneTexture, uv).rgb;
	float ao = texture(ssaoTexture, uv).r;
	float weight = texture(prepassNormals, uv).a;

	// Scene is lit-only when SSAO defers distance fog (forwardDistanceFog off).
	vec3 litAo = scene * mix(1.0, ao, ssaoIntensity * weight);

	vec3 result = litAo;
	if (fogEnabled > 0)
	{
		float depth = texture(prepassDepth, uv).r;
		if (depth < SKY_DEPTH_THRESHOLD)
		{
			float fogRange = fogEnd - fogStart;
			if (abs(fogRange) > 1e-3)
			{
				float viewDist = length(wzGetViewPosition(uv, depth, invProjectionMatrix));
				// Match terrain / tcmask forward fog exactly:
				//   f = clamp((fogEnd - dist) / (fogEnd - fogStart), 0, 1)
				//   C = mix(lit, fog, f)
				//
				// IMPORTANT: WZ binds renderState.fogBegin -> shader fogEnd and
				// renderState.fogEnd -> shader fogStart (see TerrainCombinedUniforms /
				// Draw3DShape* fog field order). Host code for this pass must use
				// the same swap so f is 0 near / 1 far with mix(lit, fog, f).
				float f = clamp((fogEnd - viewDist) / fogRange, 0.0, 1.0);
				result = mix(litAo, fogColor.rgb, f);
			}
		}
		// Sky / no prepass geometry: keep litAo (skybox already applied its own fog in ScenePass).
	}

	#ifdef NEWGL
	FragColor = vec4(result, 1.0);
	#else
	gl_FragColor = vec4(result, 1.0);
	#endif
}
