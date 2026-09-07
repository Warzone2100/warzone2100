// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	mat4 invProjectionMatrix;
	vec4 sceneUvScaleClamp;
	vec4 ssrUvScaleClamp;
	vec4 normalsUvScaleClamp;
	vec4 depthUvScaleClamp;
	float intensity;
	float F0;
	float padding0;
	float padding1;
};

uniform sampler2D sceneTexture;
uniform sampler2D ssrTexture;
uniform sampler2D prepassNormals;
uniform sampler2D prepassDepth;

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
#endif

#include "view_position.glsl"

const float SSR_WEIGHT_EPSILON = 1e-3;
const float NORMAL_LENGTH_EPSILON = 1e-5;
const float SCHLICK_EXPONENT = 5.0;
// Schlick with F0 = 0.22 still leaves facing water weak under an RTS camera.
// Floor keeps the compose mix readable. Not physical ocean F0 (~0.02).
const float FACING_FRESNEL_FLOOR = 0.45;

void writeColor(vec4 color)
{
	#ifdef NEWGL
	FragColor = color;
	#else
	gl_FragColor = color;
	#endif
}

void main()
{
	vec2 sceneUv = clamp(texCoords * sceneUvScaleClamp.xy, vec2(0.0), sceneUvScaleClamp.zw);
	vec2 ssrUv = clamp(texCoords * ssrUvScaleClamp.xy, vec2(0.0), ssrUvScaleClamp.zw);
	vec2 nUv = clamp(texCoords * normalsUvScaleClamp.xy, vec2(0.0), normalsUvScaleClamp.zw);
	vec2 dUv = clamp(texCoords * depthUvScaleClamp.xy, vec2(0.0), depthUvScaleClamp.zw);

	vec3 scene = texture(sceneTexture, sceneUv).rgb;
	vec4 ssr = texture(ssrTexture, ssrUv);
	float ssrWeight = 1.0 - texture(prepassNormals, nUv).a;
	if (ssrWeight < SSR_WEIGHT_EPSILON || ssr.a < SSR_WEIGHT_EPSILON)
	{
		writeColor(vec4(scene, 1.0));
		return;
	}

	float depth = texture(prepassDepth, dUv).r;
	vec3 viewPos = wzGetViewPosition(dUv, depth, invProjectionMatrix);
	vec3 N = texture(prepassNormals, nUv).xyz * 2.0 - 1.0;
	float nLen = length(N);
	N = (nLen < NORMAL_LENGTH_EPSILON) ? vec3(0.0, 0.0, -1.0) : (N / nLen);
	vec3 V = normalize(-viewPos);
	float ndotv = clamp(dot(N, V), 0.0, 1.0);
	float F = F0 + (1.0 - F0) * pow(1.0 - ndotv, SCHLICK_EXPONENT);
	float mixAmt = clamp(ssrWeight * max(F, FACING_FRESNEL_FLOOR) * ssr.a * intensity, 0.0, 1.0);
	writeColor(vec4(mix(scene, ssr.rgb, mixAmt), 1.0));
}
