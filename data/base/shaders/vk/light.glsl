// diffuse component of light by Lambertian reflectance
float lambertTerm(vec3 normal, vec3 lightVec) {
	return max( dot( normal, lightVec ), 0.0f);
}

// specular component of light by Blinn-Phong BRDF
float blinnTerm(vec3 normal, vec3 halfVector, float specularMap, float reflectionValue)
{
	// Ensure the dot product is not negative
	float blinnTerm = max(dot(normal, halfVector), 0.0f);

	float specularReflection = reflectionValue * (1.0f - specularMap * specularMap);
	// Prevent reflectionValue from creating a negative or zero exponent
	specularReflection = max(specularReflection, 0.0f);

	// pow(0.0, 0.0) is undefined on some GPUs and can return NaN
	if (blinnTerm == 0.0f && specularReflection == 0.0f) return 0.0f;

	return clamp(pow(blinnTerm, specularReflection), 0.0f, 1.0f);
}
