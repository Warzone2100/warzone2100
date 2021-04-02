#version 450

layout(set = 1, binding = 0) uniform sampler2D tex;
layout(set = 1, binding = 1) uniform sampler2D lightmap_tex;
layout(set = 1, binding = 2) uniform sampler2D TextureNormal; // normal map
layout(set = 1, binding = 3) uniform sampler2D TextureSpecular; // specular map

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 textureMatrix1;
	mat4 textureMatrix2;
	mat4 ModelViewMatrix;
	mat4 ModelViewProjectionMatrix;
	vec4 sunPosition;
	vec4 paramx1;
	vec4 paramy1;
	vec4 paramx2;
	vec4 paramy2;
	vec4 fogColor;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	int texture0;
	int texture1;
	int hasNormalmap;
	int hasSpecularmap;
};

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv1;
layout(location = 2) in vec2 uv2;
layout(location = 3) in float vertexDistance;
// TODO: in tangent space:
layout(location = 4) in vec3 normal;
layout(location = 5) in vec3 lightDir;
layout(location = 6) in vec3 halfVec;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 mask = color * texture(lightmap_tex, uv2);
	vec4 fragColor = mask * texture(tex, uv1);
	vec3 N;
	if (hasNormalmap != 0) {
		// TODO: N = texture(lightmap_tex, uv2).xzy * 2 - 1;
		N = normalize(normal);
	} else {
		N = normalize(normal);
	}
	vec3 L = normalize(lightDir);
	float lambertTerm = max(dot(N, L), 0.0); // diffuse lighting

	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float angle = acos(dot(H, N));
	float exponent = angle / 0.2;
	exponent = -(exponent * exponent);
	float gaussianTerm = exp(exponent);

	fragColor = fragColor*(lambertTerm*0.4 + 0.5) + mask*gaussianTerm*0.3;

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
