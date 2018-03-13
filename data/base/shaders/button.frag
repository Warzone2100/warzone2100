// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

uniform sampler2D Texture;
uniform sampler2D TextureTcmask;
uniform vec4 colour;
uniform vec4 teamcolour;
uniform int tcmask;
uniform bool alphaTest;

#if __VERSION__ >= 130
in vec2 texCoord;
#else
varying vec2 texCoord;
#endif

#if __VERSION__ >= 130
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	// Get color from texture unit 0
	#if __VERSION__ >= 130
	vec4 texColour = texture(Texture, texCoord);
	#else
	vec4 texColour = texture2D(Texture, texCoord);
	#endif

	vec4 fragColour;
	if (tcmask == 1)
	{
		// Get tcmask information from texture unit 1
		#if __VERSION__ >= 130
		vec4 mask = texture(TextureTcmask, texCoord);
		#else
		vec4 mask = texture2D(TextureTcmask, texCoord);
		#endif

		// Apply colour using grain merge with tcmask
		fragColour = (texColour + (teamcolour - 0.5) * mask.a) * colour;
	}
	else
	{
		fragColour = texColour * colour;
	}

	if (alphaTest && fragColour.a <= 0.001)
	{
		discard;
	}

	#if __VERSION__ >= 130
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
