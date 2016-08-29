#version 120
#pragma debug(on)

uniform sampler2D Texture;
uniform vec4 colour;
uniform float graphicsCycle; // a periodically cycling value for special effects

varying vec2 texCoord;

void main()
{
	vec4 texColour = texture2D(Texture, texCoord);

	gl_FragColor = texColour * colour;
}
