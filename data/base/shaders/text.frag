#version 120

uniform vec4 color;
uniform sampler2D texture;

varying vec2 uv;

void main()
{
	vec4 texColour = texture2D(texture, uv) * color.a;
	gl_FragColor = texColour * color;

	// gl_FragData[1] apparently fails to compile for some people, see #4584.
	// GL::SC(Error:High) : 0:12(2): error: array index must be < 1
	//gl_FragData[0] = texColour * color;
	//gl_FragData[1] = texColour;
}
