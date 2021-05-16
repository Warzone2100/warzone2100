#version 450

layout(set = 1, binding = 0) uniform sampler2D tex1;
layout(set = 1, binding = 1) uniform sampler2D tex2;
layout(set = 1, binding = 2) uniform sampler2D tex1_nm;
layout(set = 1, binding = 3) uniform sampler2D tex2_nm;
layout(set = 1, binding = 4) uniform sampler2D tex1_sm;
layout(set = 1, binding = 5) uniform sampler2D tex2_sm;
layout(set = 1, binding = 6) uniform sampler2D tex1_hm;
layout(set = 1, binding = 7) uniform sampler2D tex2_hm;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUV1Matrix;
	mat4 ModelUV2Matrix;
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
	float timeSec;
};

layout(location = 1) in vec2 uv1;
layout(location = 2) in vec2 uv2;
layout(location = 3) in float vertexDistance;
layout(location = 4) in vec3 lightDir;
layout(location = 5) in vec3 halfVec;
layout(location = 6) in float depth;

layout(location = 0) out vec4 FragColor;

void main()
{
	vec4 fragColor = texture(tex1, uv1) * texture(tex2, uv2);

	vec3 N = texture(tex1_nm, uv1).xzy; // y is up in modelSpace
	if (N == vec3(0,0,0)) {
		N = vec3(0,1,0);
	} else {
		N = normalize(N * 2.0 - 1.0);
	}
	float lambertTerm = max(dot(N, lightDir), 0.0); // diffuse lighting

	// Gaussian specular term computation
	vec3 H = normalize(halfVec);
	float angle = acos(dot(H, N));
	float exponent = angle / 0.2;
	exponent = -(exponent * exponent);
	float gaussianTerm = exp(exponent) * float(lambertTerm > 0);

	vec4 gloss = texture(tex1_sm, uv1) * texture(tex2_sm, uv2);

	fragColor = fragColor*(ambientLight + diffuseLight*lambertTerm) + specularLight*gloss*gaussianTerm;

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, vec4(1), fogFactor);
	}

	FragColor = fragColor;
}
