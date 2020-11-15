// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

uniform sampler2D Texture;
uniform vec4 colour;
uniform bool alphaTest;
uniform float graphicsCycle; // a periodically cycling value for special effects

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in float vertexDistance;
in vec2 texCoord;
#else
varying float vertexDistance;
varying vec2 texCoord;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	vec4 texColour = texture(Texture, texCoord);
	#else
	vec4 texColour = texture2D(Texture, texCoord);
	#endif

	vec4 fragColour = texColour * colour;

	if (alphaTest && (fragColour.a <= 0.001))
	{
		discard;
	}
	
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1)
		{
			discard;
		}

		// Return fragment color
		fragColour = mix(fragColour, vec4(fogColor.xyz, fragColour.w), clamp(fogFactor, 0.0, 1.0));
	}

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
