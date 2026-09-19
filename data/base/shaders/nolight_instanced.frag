// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

#include "tcmask_instanced.glsl"

// constants overridden by WZ when loading shaders (do not modify here in the shader source!)
//

uniform sampler2D Texture;


#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#define FRAGMENT_INPUT in
#else
#define texture(tex,uv,bias) texture2D(tex,uv,bias)
#define FRAGMENT_INPUT varying
#endif

FRAGMENT_INPUT vec4 texCoord_vertexDistance; // vec(2) texCoord, float vertexDistance, (unused float)
FRAGMENT_INPUT vec4 colour;
FRAGMENT_INPUT vec4 packed_ecmState_alphaTest;
FRAGMENT_INPUT vec3 posViewSpace;

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

#include "distance_fog.glsl"

void main()
{
	// unpack inputs
	vec2 texCoord = vec2(texCoord_vertexDistance.x, texCoord_vertexDistance.y);
	bool alphaTest = (packed_ecmState_alphaTest.y > 0.f);

	vec4 texColour = texture(Texture, texCoord, WZ_MIP_LOAD_BIAS);

	vec4 fragColour = texColour * colour;
	if (fogRange.z > 0.5 && fogOutput != WZ_FOG_OUTPUT_DISABLED)
	{
		float fogAmount = wzDistanceFogAmount(length(posViewSpace), fogRange.x, fogRange.y);
		fragColour.rgb = wzApplyForwardFog(fragColour.rgb, fragColour.a, fogAmount, fogColor.rgb, fogOutput);
	}

	if (alphaTest && (fragColour.a <= 0.001))
	{
		discard;
	}

	#ifdef NEWGL
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
