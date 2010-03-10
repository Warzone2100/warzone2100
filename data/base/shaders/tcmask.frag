#pragma debug(on)

#ifdef FOG_ENABLED
varying float vertexDistance;
#endif

uniform sampler2D Texture0;
uniform sampler2D Texture1;
uniform vec4 teamcolour;

void main(void)
{
	vec4 colour, mask;

	// Get color and tcmask information from TIUs 0-1
	colour = texture2D(Texture0, gl_TexCoord[0].st);
	mask  = texture2D(Texture1, gl_TexCoord[0].st);
	
	// Apply color using "Merge grain" within tcmask
	gl_FragColor = (colour + (teamcolour - 0.5) * mask.a) * gl_Color;
	
#ifdef FOG_ENABLED
	// Calculate linear fog
	float fogFactor = (gl_Fog.end - vertexDistance) / (gl_Fog.end - gl_Fog.start);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	
	// Return fragment color
	gl_FragColor = mix(gl_Fog.color, gl_FragColor, fogFactor);
#endif
}