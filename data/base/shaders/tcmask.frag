#version 120
#pragma debug(on)

varying float vertexDistance;

uniform sampler2D Texture0;
uniform sampler2D Texture1;
uniform vec4 teamcolour;
uniform int tcmask;
uniform int fogEnabled;

void main(void)
{
	vec4 colour, mask;

	// Get color from texture unit 0
	colour = texture2D(Texture0, gl_TexCoord[0].st);

	if (tcmask == 1)
	{
		// Get tcmask information from texture unit 1
		mask  = texture2D(Texture1, gl_TexCoord[0].st);
	
		// Apply color using grain merge with tcmask
		gl_FragColor = (colour + (teamcolour - 0.5) * mask.a) * gl_Color;
	}
	else
	{
		gl_FragColor = colour * gl_Color;
	}

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (gl_Fog.end - vertexDistance) / (gl_Fog.end - gl_Fog.start);
		fogFactor = clamp(fogFactor, 0.0, 1.0);
	
		// Return fragment color
		gl_FragColor = mix(gl_Fog.color, gl_FragColor, fogFactor);
	}
}
