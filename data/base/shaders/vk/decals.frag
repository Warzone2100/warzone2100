#version 450

layout(set = 1, binding = 0) uniform sampler2D tex;
layout(set = 1, binding = 1) uniform sampler2D lightmap_tex;
layout(set = 1, binding = 2) uniform sampler2D TextureNormal;
layout(set = 1, binding = 3) uniform sampler2D TextureSpecular;
layout(set = 1, binding = 4) uniform sampler2D TextureHeight;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUVLightmapMatrix;
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
};

layout(location = 0) in vec2 uv_tex;
layout(location = 1) in vec2 uv_lightmap;
layout(location = 2) in float vertexDistance;
layout(location = 3) in vec3 lightDir;
layout(location = 4) in vec3 halfVec;
//layout(location = 5) flat in int tile;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec3 N = normalize(texture(TextureNormal, uv_tex).xyz * 2.0 - 1.0);
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); // diffuse lighting

	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float angle = acos(dot(H, N));
	float exponent = angle / 0.2;
	exponent = -(exponent * exponent);
	float gaussianTerm = exp(exponent) * float(lambertTerm > 0);
	vec4 gloss = texture(TextureSpecular, uv_tex);

	vec4 texColor = texture(tex, uv_tex);
	vec4 fragColor = texColor*(ambientLight + diffuseLight*lambertTerm) + vec4(1,1,1,texColor.a)*specularLight*gloss*gaussianTerm;
	fragColor *= texture(lightmap_tex, uv_lightmap);

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, vec4(fogColor.xyz, fragColor.w), fogFactor);
	}
	
	FragColor = fragColor;
}
