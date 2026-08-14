// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.40 - 1.50 core.)

layout(std140) uniform cbuffer {
	float ssaoIntensity;
	float padding0;
	float padding1;
	float padding2;
	vec4 sceneUvScaleClamp;
	vec4 aoUvScaleClamp;
};
uniform sampler2D sceneTexture;
uniform sampler2D ssaoTexture;
uniform sampler2D prepassNormals;

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
	vec2 sceneUv = clamp(texCoords * sceneUvScaleClamp.xy, vec2(0.0), sceneUvScaleClamp.zw);
	vec2 aoUv = clamp(texCoords * aoUvScaleClamp.xy, vec2(0.0), aoUvScaleClamp.zw);
	vec3 scene = texture(sceneTexture, sceneUv).rgb;
	float ao = texture(ssaoTexture, aoUv).r;
	float weight = texture(prepassNormals, sceneUv).a;
	vec3 litAo = scene * mix(1.0, ao, ssaoIntensity * weight);

	#ifdef NEWGL
	FragColor = vec4(litAo, 1.0);
	#else
	gl_FragColor = vec4(litAo, 1.0);
	#endif
}
