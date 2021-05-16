#version 450

layout(set = 1, binding = 0) uniform sampler2D tex;
layout(set = 1, binding = 1) uniform sampler2D lightmap_tex;
layout(set = 1, binding = 2) uniform sampler2D TextureNormal; // normal map
layout(set = 1, binding = 3) uniform sampler2D TextureSpecular; // specular map
layout(set = 1, binding = 4) uniform sampler2D TextureHeight;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUVMatrix;
	mat4 ModelUVLightMatrix;
	vec4 cameraPos; // in modelSpace
	vec4 sunPos; // in modelSpace, normalized
	vec4 emissiveLight; // light colors/intensity
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
	vec4 fogColor;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	int hasNormalmap;
	int hasSpecularmap;
	int hasHeightmap;
};

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec2 uvLight;
layout(location = 3) in float vertexDistance;
// Light in tangent space:
layout(location = 4) in vec3 lightDir;
layout(location = 5) in vec3 halfVec;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec3 N;
	if (hasNormalmap != 0) {
		vec3 normalFromMap = texture(TextureNormal, uv).xyz;
		N = normalize(normalFromMap * 2.0 - 1.0);
		N.y = -N.y;
	} else {
		N = vec3(0,0,1); // in tangent space so normal to xy plane
	}
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); // diffuse lighting

	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float angle = acos(dot(H, N));
	float exponent = angle / 0.5;
	exponent = -(exponent * exponent);
	float gaussianTerm = exp(exponent) * float(lambertTerm > 0);

	vec4 gloss;
	if (hasSpecularmap != 0) {
		gloss = texture(TextureSpecular, uv);
	} else {
		gloss = vec4(vec3(0.1), 1);
	}

	vec4 fragColor = texture(tex, uv)*(ambientLight + diffuseLight*lambertTerm) + specularLight*gloss*gaussianTerm;
	fragColor *= texture(lightmap_tex, uvLight);
	fragColor.a = color.a;

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);

		if(fogFactor > 1.0)
		{
			discard;
		}

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), clamp(fogFactor, 0.0, 1.0));
	}
	
	FragColor = fragColor;
}
