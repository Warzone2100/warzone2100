// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D Texture;

// xy scales full-texture UVs down to the rendered sub-rect of the source,
// zw clamps just inside its edge so bilinear filtering cannot bleed in
// texels from the unrendered region (both are identity-like at full size)
layout(std140) uniform cbuffer {
	vec4 uvScaleClamp;
};

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv) texture2D(tex,uv)
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
	vec2 uv = min(texCoords * uvScaleClamp.xy, uvScaleClamp.zw);
	vec3 texColour = texture(Texture, uv).rgb;

	#ifdef NEWGL
	FragColor = vec4(texColour, 1.0);
	#else
	gl_FragColor = vec4(texColour, 1.0);
	#endif
}
