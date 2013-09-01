#version 120
#pragma debug(on)

varying float vertexDistance;
varying vec3 normal, lightDir, eyeVec;

uniform sampler2D Texture0; // diffuse
uniform sampler2D Texture1; // tcmask
uniform sampler2D Texture2; // normal map
uniform vec4 teamcolour; // the team colour of the model
uniform int tcmask; // whether a tcmask texture exists for the model
uniform int normalmap; // whether a normal map exists for the model
uniform int fogEnabled; // whether fog is enabled
uniform bool ecmEffect; // whether ECM special effect is enabled
uniform float graphicsCycle; // a periodically cycling value for special effects

void main(void)
{
	vec4 mask, colour;
	vec4 light = gl_FrontLightModelProduct.sceneColor + gl_LightSource[0].ambient;
	vec3 N = normalize(normal);
	vec3 L = normalize(lightDir);
	float lambertTerm = dot(N, L);
	if (lambertTerm > 0.0)
	{
		light += gl_LightSource[0].diffuse * lambertTerm;
		vec3 E = normalize(eyeVec);
		vec3 R = reflect(-L, N);
		float specular = pow(max(dot(R, E), 0.0), 10); // 10 is an arbitrary value for now
		light += gl_LightSource[0].specular * specular;
	}

	// Get color from texture unit 0, merge with lighting
	colour = texture2D(Texture0, gl_TexCoord[0].st) * light;

	if (tcmask == 1)
	{
		// Get tcmask information from texture unit 1
		mask = texture2D(Texture1, gl_TexCoord[0].st);
	
		// Apply color using grain merge with tcmask
		gl_FragColor = (colour + (teamcolour - 0.5) * mask.a) * gl_Color;
	}
	else
	{
		gl_FragColor = colour * gl_Color;
	}

	if (ecmEffect)
	{
		gl_FragColor.a = 0.45 + 0.225 * graphicsCycle;
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
