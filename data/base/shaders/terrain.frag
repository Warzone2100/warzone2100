// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D tex;
uniform sampler2D lightmap_tex;
uniform sampler2D TextureNormal; // normal map
uniform sampler2D TextureSpecular; // specular map

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;
uniform int hasNormalmap; // whether a normal map exists for the model
uniform int hasSpecularmap; // whether a specular map exists for the model

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
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

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	vec4 fragColor = color * texture(tex, uv1) * texture(lightmap_tex, uv2);
	#else
	vec4 fragColor = color * texture2D(tex, uv1) * texture2D(lightmap_tex, uv2);
	#endif
	
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1.f)
		{
			discard;
		}

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), clamp(fogFactor, 0.0, 1.0));
	}

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = fragColor;
	#else
	gl_FragColor = fragColor;
	#endif
}
