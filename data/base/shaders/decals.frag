#version 120

uniform sampler2D tex;
uniform sampler2D lightmap_tex;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

varying vec2 uv_tex;
varying vec2 uv_lightmap;
varying float vertexDistance;

void main()
{
	vec4 fragColor = texture2D(tex, uv_tex) * texture2D(lightmap_tex, uv_lightmap);
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
