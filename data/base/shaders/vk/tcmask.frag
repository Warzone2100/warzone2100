#version 450
//#pragma debug(on)

layout(set = 1, binding = 0) uniform sampler2D Texture; // diffuse
layout(set = 1, binding = 1) uniform sampler2D TextureTcmask; // tcmask
layout(set = 1, binding = 2) uniform sampler2D TextureNormal; // normal map
layout(set = 1, binding = 3) uniform sampler2D TextureSpecular; // specular map

layout(std140, set = 0, binding = 0) uniform cbuffer
{
	vec4 colour;
	vec4 teamcolour; // the team colour of the model
	float stretch;
	int tcmask; // whether a tcmask texture exists for the model
	int fogEnabled; // whether fog is enabled
	int normalmap; // whether a normal map exists for the model
	int specularmap; // whether a specular map exists for the model
	bool ecmEffect; // whether ECM special effect is enabled
	int alphaTest;
	float graphicsCycle; // a periodically cycling value for special effects
	mat4 ModelViewMatrix;
	mat4 ModelViewProjectionMatrix;
	mat4 NormalMatrix;
	vec4 lightPosition;
	vec4 sceneColor;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float fogEnd;
	float fogStart;
	vec4 fogColor;
};

layout(location  = 0) in float vertexDistance;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 lightDir;
layout(location = 3) in vec3 eyeVec;
layout(location = 4) in vec2 texCoord;

layout(location = 0) out vec4 FragColor;

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
	vec4 texColour = texture(Texture, texCoord) * light;

	vec4 fragColour;
	if (tcmask == 1)
	{
		// Get tcmask information from texture unit 1
		vec4 mask = texture(TextureTcmask, texCoord);

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

	if (alphaTest > 0 && (fragColour.a <= 0.001))
	{
		discard;
	}

	FragColor = fragColour;
}
