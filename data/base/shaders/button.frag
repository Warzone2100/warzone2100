#version 120
#pragma debug(on)

uniform sampler2D Texture;
uniform sampler2D TextureTcmask;
uniform vec4 colour;
uniform vec4 teamcolour;
uniform int tcmask;

varying vec2 texCoord;

void main()
{
	// Get color from texture unit 0
	vec4 texColour = texture2D(Texture, texCoord);

	if (tcmask == 1)
	{
		// Get tcmask information from texture unit 1
		vec4 mask = texture2D(TextureTcmask, texCoord);

		// Apply colour using grain merge with tcmask
		gl_FragColor = (texColour + (teamcolour - 0.5) * mask.a) * colour;
	}
	else
	{
		gl_FragColor = texColour * colour;
	}
}
