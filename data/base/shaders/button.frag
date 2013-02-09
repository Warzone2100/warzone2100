#version 120
#pragma debug(on)

uniform sampler2D Texture0;
uniform sampler2D Texture1;
uniform vec4 teamcolour;
uniform int tcmask;

void main(void)
{
	vec4 mask, colour;

	// Get color from texture unit 0
	colour = texture2D(Texture0, gl_TexCoord[0].st);

	if (tcmask == 1)
	{
		// Get tcmask information from texture unit 1
		mask = texture2D(Texture1, gl_TexCoord[0].st);
	
		// Apply color using grain merge with tcmask
		gl_FragColor = (colour + (teamcolour - 0.5) * mask.a) * gl_Color;
	}
	else
	{
		gl_FragColor = colour * gl_Color;
	}
}
