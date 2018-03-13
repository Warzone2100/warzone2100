// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

uniform sampler2D Texture;
uniform vec4 colour;
uniform bool alphaTest;
uniform float graphicsCycle; // a periodically cycling value for special effects

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
	#if __VERSION__ >= 130
	vec4 texColour = texture(Texture, texCoord);
	#else
	vec4 texColour = texture2D(Texture, texCoord);
	#endif

	vec4 fragColour = texColour * colour;

	if (alphaTest && (fragColour.a <= 0.001))
	{
		discard;
	}

	#if __VERSION__ >= 130
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
