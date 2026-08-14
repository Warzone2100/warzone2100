// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
//

uniform sampler2D lightmap_tex;

layout(std140) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	vec4 paramx1;
	vec4 paramy1;
	vec4 paramx2;
	vec4 paramy2;
	mat4 textureMatrix1;
	mat4 textureMatrix2;
	vec4 fogColor;
	int fogEnabled;
	float fogEnd;
	float fogStart;
};

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#endif

#ifdef NEWGL
in vec2 uv2;
in float vertexDistance;
#else
varying vec2 uv2;
varying float vertexDistance;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	vec4 fragColor = texture(lightmap_tex, uv2, 0.f);
	
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1.f)
		{
			discard;
		}

		// Return fragment color
		fragColor = fragColor;
	}

	#ifdef NEWGL
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
