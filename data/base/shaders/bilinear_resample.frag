// Version directive is set by Warzone when loading the shader.

layout(std140) uniform cbuffer {
	vec4 uvScaleClamp;
};
uniform sampler2D sourceTexture;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in vec2 texCoords;
out vec4 FragColor;
#define textureSample texture
#else
varying vec2 texCoords;
#define FragColor gl_FragColor
#define textureSample texture2D
#endif

void main()
{
	vec2 uv = clamp(texCoords * uvScaleClamp.xy, vec2(0.0), uvScaleClamp.zw);
	// Preserve the full value contract. R8 destinations retain red; RGBA effects
	// retain color and confidence without needing effect-specific resamplers.
	FragColor = textureSample(sourceTexture, uv);
}
