// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

//#pragma debug(on)

uniform sampler2D Texture; // diffuse
uniform sampler2D TextureTcmask; // tcmask
uniform sampler2D TextureNormal; // normal map
uniform sampler2D TextureSpecular; // specular map
uniform vec4 colour;
uniform vec4 teamcolour; // the team colour of the model
uniform int tcmask; // whether a tcmask texture exists for the model
uniform int normalmap; // whether a normal map exists for the model
uniform int specularmap; // whether a specular map exists for the model
uniform bool ecmEffect; // whether ECM special effect is enabled
uniform bool alphaTest;
uniform float graphicsCycle; // a periodically cycling value for special effects

uniform vec4 sceneColor;
uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 specular;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
in float vertexDistance;
in vec3 normal, lightDir, eyeVec;
in vec2 texCoord;
#else
varying float vertexDistance;
varying vec3 normal, lightDir, eyeVec;
varying vec2 texCoord;
#endif

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	vec4 light = sceneColor * vec4(.2, .2, .2, 1.) + ambient;
	vec3 N = normalize(normal);
	vec3 L = normalize(lightDir);
	float lambertTerm = dot(N, L);
	if (lambertTerm > 0.0)
	{
		light += diffuse * lambertTerm;
		vec3 E = normalize(eyeVec);
		vec3 R = reflect(-L, N);
		float s = pow(max(dot(R, E), 0.0), 10.0); // 10 is an arbitrary value for now
		light += specular * s;
	}

	// Get color from texture unit 0, merge with lighting
	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	vec4 texColour = texture(Texture, texCoord) * light;
	#else
	vec4 texColour = texture2D(Texture, texCoord) * light;
	#endif

	vec4 fragColour;
	if (tcmask == 1)
	{
		// Get tcmask information from texture unit 1
		#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
		vec4 mask = texture(TextureTcmask, texCoord);
		#else
		vec4 mask = texture2D(TextureTcmask, texCoord);
		#endif

		// Apply color using grain merge with tcmask
		fragColour = (texColour + (teamcolour - 0.5) * mask.a) * colour;
	}
	else
	{
		fragColour = texColour * colour;
	}

	if (ecmEffect)
	{
		fragColour.a = 0.45 + 0.225 * graphicsCycle;
	}

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColour = mix(fogColor, fragColour, fogFactor);
	}

	if (alphaTest && (fragColour.a <= 0.001))
	{
		discard;
	}

	#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
	FragColor = fragColour;
	#else
	gl_FragColor = fragColour;
	#endif
}
