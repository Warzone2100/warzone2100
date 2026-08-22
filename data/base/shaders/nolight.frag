// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
layout(std140) uniform globaluniforms {
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ShadowMapMVPMatrix;
	vec4 cameraPos;
	vec4 lightPosition;
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 fogColor;
	vec4 fogRange;
	float graphicsCycle;
	float WZ_MIP_LOAD_BIAS;
	float pad0;
	float pad1;
};

layout(std140) uniform meshuniforms {
	int tcmask;
	int normalmap;
	int specularmap;
	int hasTangents;
	int fogOutput;
};

layout(std140) uniform instanceuniforms {
	mat4 ModelMatrix;
	mat4 NormalMatrix;
	vec4 colour;
	vec4 teamcolour;
	float stretch;
	float animFrameNumber;
	int ecmEffect;
	int alphaTest;
};
//

uniform sampler2D Texture;


#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#endif

#ifdef NEWGL
in vec3 posViewSpace;
in vec2 texCoord;
#else
varying vec3 posViewSpace;
varying vec2 texCoord;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

#include "distance_fog.glsl"

void main()
{
	vec4 texColour = texture(Texture, texCoord, WZ_MIP_LOAD_BIAS);

	vec4 fragColour = texColour * colour;
	if (fogRange.z > 0.5 && fogOutput != WZ_FOG_OUTPUT_DISABLED)
	{
		float fogAmount = wzDistanceFogAmount(length(posViewSpace), fogRange.x, fogRange.y);
		fragColour.rgb = wzApplyForwardFog(fragColour.rgb, fragColour.a, fogAmount, fogColor.rgb, fogOutput);
	}

	if (alphaTest > 0 && (fragColour.a <= 0.001))
	{
		discard;
	}

	#ifdef NEWGL
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
