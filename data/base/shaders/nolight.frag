#version 120
#pragma debug(on)

uniform sampler2D Texture;
uniform vec4 colour;
uniform bool alphaTest;
uniform float graphicsCycle; // a periodically cycling value for special effects

varying vec2 texCoord;

void main()
{
	vec4 texColour = texture2D(Texture, texCoord);

	vec4 fragColour = texColour * colour;

	if (alphaTest && (fragColour.a <= 0.001))
	{
		discard;
	}

	gl_FragColor = fragColour;
}
