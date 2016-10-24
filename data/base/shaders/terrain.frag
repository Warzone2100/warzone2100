#version 120

uniform sampler2D tex;
uniform sampler2D lightmap_tex;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

varying vec4 color;
varying vec2 uv1;
varying vec2 uv2;
varying float vertexDistance;

void main()
{
	vec4 fragColor = color * texture2D(tex, uv1) * texture2D(lightmap_tex, uv2);
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fogColor, fragColor, fogFactor);
	}
	gl_FragColor = fragColor;
}
