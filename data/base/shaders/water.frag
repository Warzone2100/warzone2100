// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D tex1;
uniform sampler2D tex2;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if __VERSION__ >= 130
in vec4 color;
in vec2 uv1;
in vec2 uv2;
in float vertexDistance;
#else
varying vec4 color;
varying vec2 uv1;
varying vec2 uv2;
varying float vertexDistance;
#endif

#if __VERSION__ >= 130
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#if __VERSION__ >= 130
	vec4 fragColor = texture(tex1, uv1) * texture(tex2, uv2);
	#else
	vec4 fragColor = texture2D(tex1, uv1) * texture2D(tex2, uv2);
	#endif
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(vec4(1.), fragColor, fogFactor);
	}
	#if __VERSION__ >= 130
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
